<?php
/*------------------------------------------------------------------------------
 The Websms class provides functions for creating Asterisk call files

 <websmsd.php>
 listen()
   permitted()
   echo()
   post_mess($post)
 answer($status)


 <websms.php>
 query($args)
   args_mess($args)
   curl_init()
   curl_data($post)
   curl_auth($post)
   curl_send()
   check($response)
*/
class Websms {
	const DEFAULT_CONF_FILE = '/etc/asterisk/websms.conf';
	const DEFAULT_CONFD_VALS = [
		'key_to'          => "To",
		'key_from'        => "From",
		'key_body'        => "Body",
		'key_echo'        => null,
		'key_account'     => null,
		'report_success'  => "",
		'permit_addr'     => null,
		'proxy_addr'      => "172.16.0.0/12",
		'proxy_header'    => "HTTP_X_FORWARDED_FOR",
	];
	const DEFAULT_CONF_VALS = [
		'host'            => "https://api.example.com",
		'path'            => "/sms/send/",
		'key_to'          => "To",
		'key_from'        => "From",
		'key_body'        => "Body",
		'auth_user'       => "usename",
		'auth_passwd'     => "passwd",
		'auth_method'     => "basic",
		'response_check'  => null,
		'number_format'   => "",
	];
	private $config;
	private $message;
	private $curl;
	public $debug = false;
	public function __construct($config = null, array $optconfig = []) {
		if (is_string($config) !== true) {
			$config = self::DEFAULT_CONF_FILE;
		}
		$this->config['websmsd'] = self::DEFAULT_CONFD_VALS;
		$this->config['websms'] = self::DEFAULT_CONF_VALS;
		if (file_exists($config) === true) {
			$config_ini = parse_ini_file($config,true);
			$this->config['websmsd'] = array_merge($this->config['websmsd'],
				$config_ini['websmsd']);
			$this->config['websms'] = array_merge($this->config['websms'],
				$config_ini['websms']);
		}
		foreach ($optconfig as $var => $val) {
			$this->config['websms'][$var] = $val;
		}
	}
	/*--------------------------------------------------------------------------
	Read the post header data.
	Use $_POST since file_get_contents("php://input") cannot handle multipart/form-data
	@return mixed array message or boolean false
	*/
	public function listen() {
		$this->message = false;
		$post = $_POST;
		if ($this->permitted()) {
			$this->echo();
			$this->post_mess($post);
			return $this->message;
		} else {
			return false;
		}
	}
	/*--------------------------------------------------------------------------
	Respond with a status message.
	*/
	public function answer($status = true) {
		if ($status) {
			trigger_error('Inbound SMS accepted, for ('.$this->message['to'].')', E_USER_NOTICE);
			echo $this->config['websmsd']['report_success'];
		}
	}
	/*--------------------------------------------------------------------------
	Send SMS HTTP POST query. We require 3 arguments; to, from, body.
	args[0] is set to script name so drop this.

	Outline
		Parse arguments.
		Build http POST query.
		Setup curl for POST query.
		Setup curl's authentication method.
		Send POST query.
		Check response.
	*/
	public function query($args) {
		unset($args[0]);
		$poststr = $this->args_mess($args);
		if (!empty($poststr)) {
			$this->curl_init();
			$this->curl_data($poststr);
			$this->curl_auth($poststr);
			$response = $this->curl_send();
			return $this->check($response);
		} else {
			return false;
		}
	}
	/*--------------------------------------------------------------------------
	Respond to echo requests.
	The API of some ITSP, eg Zadarma, test the web server by sending an echo
	request. Let's respond and exit if we detect a echo request.
	*/
	private function echo() {
		if (!empty($this->config['websmsd']['key_echo'])
			&& isset($_GET[$this->config['websmsd']['key_echo']])) {
			trigger_error('Received echo request ('.$_GET[$this->config['websmsd']['key_echo']].')', E_USER_NOTICE);
			exit($_GET[$this->config['websmsd']['key_echo']]);
		}
	}
	/*--------------------------------------------------------------------------
	Evaluates POST request data and updates $this->message. Parameters are
	json decoded and searched recursively.
	@param array POST request parameters
	@return void
	*/
	private function post_mess($post) {
		if (empty($post)) {
			trigger_error("No POST header data", E_USER_WARNING);
			return false;
		}
		array_walk_recursive($post, function(&$val) {
			$json = json_decode($val,true);
			if (!empty($json)) $val = $json;
		});
		array_walk_recursive($post, function($val,$key) {
			if ($key === $this->config['websmsd']['key_to'])   $this->message['to']   = $val;
			if ($key === $this->config['websmsd']['key_from']) $this->message['from'] = $val;
			if ($key === $this->config['websmsd']['key_body']) $this->message['body'] = $val;
		});
		if (empty($this->message['to']) || empty($this->message['from']) || empty($this->message['body'])) {
			trigger_error(sprintf("Did not get all required POST data (%s)",
				json_encode($post)), E_USER_WARNING);
		}
	}
	/*--------------------------------------------------------------------------
	Initially assume 'REMOTE_ADDR' is the real IP of the HTTP client. But if it
	is a 'proxy_addr' then trust that  'proxy_header' hold the real IP of the
	client instead. Now that we have the real IP of the client check if it is
	permitted.
	@return Boolean true if $saddr is permitted otherwise false
	*/
	private function permitted() {
		if (isset($this->config['websmsd']['permit_addr'])) {
			$saddr = $_SERVER['REMOTE_ADDR'];
			if ($this->cmp_cidr($saddr,$this->config['websmsd']['proxy_addr'])) {
				$saddr = $_SERVER[$this->config['websmsd']['proxy_header']];
			}
			$permit = $this->cmp_cidr($saddr,$this->config['websmsd']['permit_addr']);
			$permitstr = $permit ? 'permitted' : 'denied';
			trigger_error('HTTP request ('.$saddr.') '.$permitstr, E_USER_NOTICE);
			return $permit;
		} else {
			return true;
		}
	}
	/*--------------------------------------------------------------------------
	@param string $ip        addr to test eg, 216.245.217.2
	@param string $csvcidrs  comma separated list of CIDR ranges
	                          eg, 185.45.152.42,3.104.90.0/24,3.1.77.0/24
	@return Boolean true if $ip matches any range in $csvcidrs
	*/
	public function cmp_cidr($ip,$csvcidrs) {
		$cidrs = explode(',',$csvcidrs);
		foreach ($cidrs as $cidr) {
			$blknmask = explode('/',$cidr);
			$blk = $blknmask[0];
			if (isset($blknmask[1])) {
				$mask = $blknmask[1];
			} else {
				$mask = 32;
			}
			$blkbin = ip2long($blk) >> (32 - $mask);
			$ipbin  = ip2long($ip) >> (32 - $mask);
			if ($ipbin === $blkbin) return true;
		}
		return false;
	}
	/*--------------------------------------------------------------------------
	receive an additional header "Signature",
	$signatureTest = base64_encode(hash_hmac('sha1', $_POST['result'], API_SECRET));
	@param array $post HTTP POST
	@return Boolean true if HTTP request was verified
	*/
	private function verify($post) {
		$method = $this->config['websmsd']['auth_method'];
		$passwd = $this->config['websmsd']['auth_passwd'];
		switch ($method) {
		case '':
			return true;
			break;
		case 'zadarma':
			$result = $post['result'];
			$sign = $post['Signature'];
			$sign_expected = base64_encode(hash_hmac('sha1', $result, $passwd));
			return ($sign === $sign_expected);
			break;
		default:
			trigger_error("Unknown method (auth_method=$method)", E_USER_WARNING);
			return true;
			break;
		}
	}
	/*--------------------------------------------------------------------------
	Parse arguments.
	We require 3 arguments; to, from, body.
	Build http POST query, from arguments, sort and URL-encode.
	*/
	private function args_mess($args) {
		if (count($args) === 3) {
			$keys = [ $this->config['websms']['key_to'],
				$this->config['websms']['key_from'],
				$this->config['websms']['key_body'] ];
			$vals = [ $this->filter_number($args[1]),
				$this->filter_number($args[2]),
				$args[3] ];
			$this->message = array_combine($keys,$vals);
			ksort($this->message);
			$this->debug($this->message);
			$post = http_build_query($this->message);
			$this->debug($post);
			return $post;
		} else {
			$strings = implode(',',$args);
			trigger_error("We did not get exactly 3 arguments; to, from, body ($strings)", E_USER_WARNING);
			return false;
		}
	}
	/*--------------------------------------------------------------------------
	Sanitize telephone numbers.
	@param string $number
	@return string $number
	*/
	private function filter_number($number) {
		switch ($this->config['websms']['number_format']) {
		case 'no+':
		case 'omit+':
			return preg_replace('/[+]/', '', $number);
		default:
			return $number;
		}
	}
	/*--------------------------------------------------------------------------
	Init curl.
	*/
	private function curl_init() {
		$url = $this->config['websms']['host'].$this->config['websms']['path'];
		$this->curl = curl_init($url);
	}
	/*--------------------------------------------------------------------------
	Setup curl for POST query.
	*/
	private function curl_data($poststr) {
		curl_setopt($this->curl, CURLOPT_RETURNTRANSFER, true);
//		curl_setopt($this->curl, CURLOPT_CUSTOMREQUEST, 'POST');
		curl_setopt($this->curl, CURLOPT_POST, true);
		curl_setopt($this->curl, CURLOPT_POSTFIELDS, $poststr);
		curl_setopt($this->curl, CURLOPT_SSL_VERIFYPEER, false);
		curl_setopt($this->curl, CURLOPT_SSL_VERIFYHOST, false);
//		curl_setopt($this->curl, CURLINFO_HEADER_OUT, true); // debug only
	}
	/*--------------------------------------------------------------------------
	Setup curl's authentication method.
	Currently we support:

	'basic'
		basic access authentication, see, wikipedia.org/wiki/Basic_access_authentication,
		with headers like: Authorization: Basic <credentials>

	'zadarma'
		Zadarma's unique authentication method, see, zadarma.com/en/support/api,
		with headers like: Authorization: <user_key>:<signature>
	*/
	private function curl_auth($poststr) {
		$method = $this->config['websms']['auth_method'];
		$user = $this->config['websms']['auth_user'];
		$passwd = $this->config['websms']['auth_passwd'];
		$path = $this->config['websms']['path'];
		switch ($method) {
		case 'basic':
			curl_setopt($this->curl, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
			curl_setopt($this->curl, CURLOPT_USERPWD, "$user:$passwd");
			break;
		case 'zadarma':
			$signature = base64_encode(hash_hmac('sha1', $path .
				$poststr . md5($poststr), $passwd));
			curl_setopt($this->curl, CURLOPT_HTTPHEADER,
				array('Authorization: ' . $user . ':' . $signature));
			break;
		default:
			trigger_error("Unknown method (auth_method=$method)", E_USER_WARNING);
			break;
		}
	}
	/*--------------------------------------------------------------------------
	Send POST query,
	read response and close.
	*/
	private function curl_send() {
		$response_json = curl_exec($this->curl);
		if (!empty($response_json)) {
			$response = json_decode($response_json,true);
		} else {
			$response = false;
			trigger_error("Curl error: ".curl_error($this->curl));
		}
		$curl_info = curl_getinfo($this->curl);
		curl_close($this->curl);
		//$this->debug($curl_info);
		$this->debug($response_json);
		return $response;
	}
	/*--------------------------------------------------------------------------
	Check response and set exit code accordingly.
	the response check is controlled by the variable response_check. It can be:
	= null\""; No check, we will always exit with status true
	= "key=value" If value of key in response is equal to value exit true otherwise false
	= "/pattern/"; If pattern matches response exit with status true otherwise false
	{
		"status":"success",
		"messages":1,
		"cost":0.24,
		"currency":"USD"
	}
	*/
	private function check($response) {
		$check = $this->config['websms']['response_check'];
		$host = $this->config['websms']['host'];
		$user = $this->config['websms']['auth_user'];
		if (empty($response)) return false;
		if (!empty($check)) {
			if (strpos($check, '=') !== false) { // "key=value"
				list($test_key, $test_value) = explode('=',$check);
				if (@$response[$test_key] !== $test_value) {
					trigger_error("Called ($host) but return key ($test_key = $response[$test_key] != $test_value)", E_USER_WARNING);
					trigger_error(sprintf("Response was: %s",
						json_encode($response)));
					return false;
				}
			} else { // "/pattern/"
				if (!preg_match($check,$response)) {
					trigger_error("Called ($host) but return not matched ($response != $check)", E_USER_WARNING);
					trigger_error(sprintf("Response was: %s",
						json_encode($response)));
					return false;
				}
			}
			trigger_error("Outbound SMS sent (host=$host,user=$user) successfully", E_USER_NOTICE);
		} else {
			trigger_error("Outbound SMS sent (host=$host,user=$user)", E_USER_NOTICE);
		}
		return true;
	}
	public function debug($data, $debug = false) {
		if($debug || $this->debug) {
			var_dump($data);
		}
	}
}
?>
