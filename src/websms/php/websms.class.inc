<?php
/*------------------------------------------------------------------------------
 The Websms class provides functions for creating Astrisk call files

 <websmsd.php>
 listen()
   filter($addr)
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
   check($responce)

------------------------------------------------------------------------------*/
class Websms {
	const DEFAULT_CONF_FILE = '/etc/asterisk/websms.conf';
	const DEFAULT_CONFD_VALS = [
		'key_to'          => "To",
		'key_from'        => "From",
		'key_body'        => "Body",
		'key_echo'        => null,
		'key_account'     => null,
		'report_success'  => "",
		'permit'          => null
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
		'responce_check'  => null
	];
	private $config;
	private $message;
	private $curl;
	public $debug = true;
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
	/**
	* Read the post header data.
	* Also log warnings if any header data is missing.
	*/
	public function listen() {
		$this->message = false;
		$post = file_get_contents("php://input");
		if ($this->firewall($_SERVER['REMOTE_ADDR'])) {
			$this->echo();
			$this->post_mess($post);
			return $this->message;
		} else {
			return false;
		}
	}
	/**
	* Respond with a status message.
	*/
	public function answer($status = true) {
		if ($status) {
			trigger_error('Inbount SMS accepted, for ('.$this->message['to'].')', E_USER_NOTICE);
			echo $this->config['websmsd']['report_success'];
		}
	}
	/**
	* Send SMS HTTP POST query. We require 3 arguments; to, from, body.
	* args[0] is set to script name so drop this.
	*
	* Outline
	* Parse arguments.
	* Build http POST query.
	* Setup curl for POST query.
	* Setup curl's authentication method.
	* Send POST query.
	* Check responce.
	*/
	public function query($args) {
		unset($args[0]);
		$post = $this->args_mess($args);
		if (!empty($post)) {
			$this->curl_init();
			$this->curl_data($post);
			$this->curl_auth($post);
			$responce = $this->curl_send();
			return $this->check($responce);
		} else {
			return false;
		}
	}
	/**
	* Respond to echo requests.
	* The API of some ITSP, eg Zadarma, test the web server by sending an echo
	* request. Let's respond and exit if we detect a echo request.
	*/
	private function echo() {
		if (!empty($this->config['websmsd']['key_echo']) 
			&& isset($_GET[$this->config['websmsd']['key_echo']])) {
			trigger_error('Received echo request ('.$_GET[$this->config['websmsd']['key_echo']].')', E_USER_NOTICE);
			exit($_GET[$this->config['websmsd']['key_echo']]);
		}
	}
	private function post_mess($post) {
		parse_str($post, $data);
		if (empty($data)) {
			trigger_error("No POST header data", E_USER_WARNING);
			return false;
		}
		if (!empty($data[$this->config['websmsd']['key_to']]) &&
			!empty($data[$this->config['websmsd']['key_from']]) &&
			!empty($data[$this->config['websmsd']['key_from']])) {
			$this->message = [
				'to'   => $data[$this->config['websmsd']['key_to']],
				'from' => $data[$this->config['websmsd']['key_from']],
				'body' => $data[$this->config['websmsd']['key_from']]
			];
		} else {
			trigger_error("Did not get all required POST data ($post)", E_USER_WARNING);
		}
	}
	private function firewall($ip) {
		if (isset($this->config['websmsd']['permit'])) {
			return $this->cmp_cidr($ip,$this->config['websmsd']['permit']);
		} else {
			return true;
		}
	}
	/*--------------------------------------------------------------------------
	 @param string $ip        addr to test eg, 216.245.217.2
	 @param string $csvcidrs  comma separated list of CIDR ranges
	                          eg, 185.45.152.42,3.104.90.0/24,3.1.77.0/24
	 @return boolean true if $ip matches any range in $csvcidrs
	--------------------------------------------------------------------------*/
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
	 @return boolean true if HTTP request was verified
	--------------------------------------------------------------------------*/
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
	/**
	* Parse arguments.
	* We require 3 arguments; to, from, body.
	* Build http POST query, from arguments, sort and URL-encode.
	*/
	private function args_mess($args) {
		if (count($args) === 3) {
			$keys = [ $this->config['websms']['key_to'],
				$this->config['websms']['key_from'],
				$this->config['websms']['key_body'] ];
			$this->message = array_combine($keys,$args);
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
	private function curl_init() {
		$url = $this->config['websms']['host'].$this->config['websms']['path'];
		$this->curl = curl_init($url);
	}
	/**
	* Setup curl for POST query.
	*/
	private function curl_data($post) {
		curl_setopt($this->curl, CURLOPT_RETURNTRANSFER, true);
		curl_setopt($this->curl, CURLOPT_POST, true);
		curl_setopt($this->curl, CURLOPT_POSTFIELDS, $post);
		curl_setopt($this->curl, CURLOPT_SSL_VERIFYPEER, false);
		curl_setopt($this->curl, CURLINFO_HEADER_OUT, true); // debug only
	}
	/**
	* Setup curl's authentication method.
	* Currently we suport:
	*
	* 'basic'
	*   basic access authentication, see, wikipedia.org/wiki/Basic_access_authentication,
	*   with headers like: Authorization: Basic <credentials>
	*
	* 'zadarma'
	*   Zadarma's uniqe authentication method, see, zadarma.com/en/support/api,
	*   with headers like: Authorization: <user_key>:<signature>
	*/
	private function curl_auth($post) {
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
				$post . md5($post), $passwd));
			curl_setopt($this->curl, CURLOPT_HTTPHEADER, array(
				'Authorization: ' . $user . ':' . $signature,
				'Content-Type: application/json'));
			break;
		default:
			trigger_error("Unknown method (auth_method=$method)", E_USER_WARNING);
			break;
		}
	}
	/**
	* Send POST query,
	* read responce and close.
	*/
	private function curl_send() {
		$responce_json = curl_exec($this->curl);
		if (!empty($responce_json)) {
			$responce = json_decode($responce_json,true);
		} else {
			$responce = false;
			trigger_error("Curl error: ".curl_error($this->curl));
		}
		$curl_info = curl_getinfo($this->curl);
		curl_close($this->curl);
		//$this->debug($curl_info);
		$this->debug($responce_json);
		return $responce;
	}
	/**
	* Check responce and set exit code accordingly.
	* the reponce check is controlled by the variable responce_check. It can be:
	* = null\""; No check, we will always exit with status true
	* = "key=value" If value of key in responce is equal to value exit true otherwize false
	* = "/pattern/"; If pattern matches responce exit with status true otherwize false
	{
	    "status":"success",
	    "messages":1,
	    "cost":0.24,
	    "currency":"USD"
	}
	*/
	private function check($responce) {
		$check = $this->config['websms']['responce_check'];
		$host = $this->config['websms']['host'];
		$user = $this->config['websms']['auth_user'];
		if (empty($responce)) return false;
		if (!empty($check)) {
			if (strpos($check, '=') !== false) { // "key=value"
				list($test_key, $test_value) = explode('=',$check);
				if (@$responce[$test_key] !== $test_value) {
					trigger_error("Called ($url) but return key ($test_key = $responce[$test_key] != $test_value)", E_USER_WARNING);
					return false;
				}
			} else { // "/pattern/"
				if (!preg_match($check,$responce)) {
					trigger_error("Called ($url) but return not matched ($responce != $check)", E_USER_WARNING);
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
