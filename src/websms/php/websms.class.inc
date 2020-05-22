<?php
/*------------------------------------------------------------------------------
 The Websms class provides functions for

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
	const DEFAULT_CONF_VALS = [
		'websms' => [
			'auth_method' => "basic",
			'auth_secret' => null,
			'auth_user'   => null,
			'key_body'    => "Body",
			'key_from'    => "From",
			'key_secret'  => null,
			'key_to'      => "To",
			'key_user'    => null,
			'resp_check'  => null,
			'url_host'    => "http://localhost",
			'url_path'    => "/",
			'val_numform' => null,
			'val_static'  => null,
			'val_unicode' => null,
		],
		'websmsd' => [
			'key_body'    => "Body",
			'key_echo'    => null,
			'key_from'    => "From",
			'key_to'      => "To",
			'prox_addr'   => "172.16.0.0/12",
			'prox_header' => "HTTP_X_FORWARDED_FOR",
			'remt_addr'   => null,
			'resp_ack'    => "",
			'url_path'    => null,
		]
	];
	private $config;
	public $index;
	private $curl;
	public $debug = false;
	public function __construct($config = null, array $optconfig = []) {
		if (is_string($config) !== true) {
			$config = self::DEFAULT_CONF_FILE;
		}
		$this->config = self::DEFAULT_CONF_VALS;
		if (file_exists($config) === true) {
			$config_ini = parse_ini_file($config,true);
			if (!empty($config_ini['websmsd']))
				$this->config['websmsd'] = array_merge($this->config['websmsd'],
					$config_ini['websmsd']);
			if (!empty($config_ini['websms']))
				$this->config['websms'] = array_merge($this->config['websms'],
					$config_ini['websms']);
		}
		foreach ($optconfig as $var => $val) {
			$this->config['websms'][$var] = $val;
		}
	}
	/*--------------------------------------------------------------------------
	Read the post header data.
	@return mixed array message or boolean false
	*/
	public function rx_query() {
		if ($this->identify_remote()) {
			$this->if_echo_exit();
			return $this->message_data();
		} else return false;
	}
	/*--------------------------------------------------------------------------
	Respond with a status message.
	@param  Boolean $status
	@param  array $message to, from, and body.
	@return void
	*/
	public function ack_query($status = true, $message) {
		if ($status) {
			trigger_error(sprintf(
				"Inbound SMS accepted, to (%s) from (%s)", $message['to'],
				$message['from']), E_USER_NOTICE);
			echo $this->config('websmsd','resp_ack');
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
	public function tx_query($args) {
		unset($args[0]);
		$query_data = $this->query_data($args);
		if (!empty($query_data)) {
			$this->val_unicode($query_data);
			$this->val_numform($query_data);
			$this->val_static($query_data);
			$this->curl_init();
			$this->curl_auth($query_data);
			$this->curl_data($query_data);
			$resp_data = $this->curl_send();
			return $this->resp_check($resp_data);
		} else {
			return false;
		}
	}
	/*--------------------------------------------------------------------------
	If remt_addr is set, match remote host and request uri, set index.
	If remt_addr isn't set but uri is, match request uri, set index.
	@return Boolean true if matched, otherwise false
	*/
	private function identify_remote() {
		$allow = true;
		$match_addr = $this->config('websmsd','remt_addr',true);
		$remt_addr = $this->remt_addr();
		$request_uri = $_SERVER['REQUEST_URI'];
		if(isset($match_addr)) {
			$allow = false;
			if (is_array($match_addr)) {
				foreach ($match_addr as $index => $value) {
					if($this->match_cidr($remt_addr,$value)) {
						$allow = true;
						$this->index = $index;
					}
				}
			} else {
				if($this->match_cidr($remt_addr,$match_addr)) $allow = true;
			}
			if($allow) trigger_error(
					sprintf("REMOTE_ADDR (%s) is allowed, using index (%s)",
					$remt_addr, $this->index), E_USER_NOTICE);
			else trigger_error(
					sprintf("REMOTE_ADDR (%s) is NOT allowed",
					$remt_addr), E_USER_WARNING);
		}
		/*
		Wait until now to call $this->config('websmsd','uri') since
		$this->index might have been set above.
		*/
		$match_uri = $this->config('websmsd','url_path',true);
		if($allow && isset($match_uri)) {
			$allow = false;
			if (is_array($match_uri)) {
				foreach ($match_uri as $index => $value) {
					if($this->match_uri($value)) {
						$allow = true;
						$this->index = $index;
					}
				}
			} else {
				if($this->match_uri($match_uri)) $allow = true;
			}
			if($allow) trigger_error(
					sprintf("REQUEST_URI (%s) is allowed, using index (%s)",
					$request_uri, $this->index), E_USER_NOTICE);
			else trigger_error(
					sprintf("REQUEST_URI (%s) is NOT allowed",
					$request_uri), E_USER_WARNING);
		}
		return $allow;
	}
	/*--------------------------------------------------------------------------
	Respond to echo requests.
	The API of some ITSP, eg Zadarma, test the web server by sending an echo
	request. Let's respond and exit if we detect a echo request.
	*/
	private function if_echo_exit() {
		if (!empty($this->config('websmsd','key_echo'))
			&& isset($_GET[$this->config('websmsd','key_echo')])) {
			trigger_error('Received echo request ('.$_GET[$this->config('websmsd','key_echo')].')', E_USER_NOTICE);
			exit($_GET[$this->config('websmsd','key_echo')]);
		}
	}
	/*--------------------------------------------------------------------------
	Evaluates POST request data and returns $message. Parameters are
	json decoded and searched recursively.
	Use $_POST since file_get_contents("php://input") cannot handle multipart/form-data
	@return array $message to, from, and body.
	*/
	private function message_data() {
		$post_data = $_POST;
		$message = [];
		if (empty($post_data)) {
			trigger_error("No POST header data", E_USER_WARNING);
			return false;
		} else {
			if($this->debug) {
				trigger_error(sprintf("POST data (%s)",json_encode($post_data)));
			}
		}
		array_walk_recursive($post_data, function(&$val) {
			$val_json = json_decode($val,true);
			if (!empty($val_json)) $val = $val_json;
		});
		array_walk_recursive($post_data, function($val,$key) use (&$message) {
			if ($key === $this->config('websmsd','key_to'))   $message['to']   = $val;
			if ($key === $this->config('websmsd','key_from')) $message['from'] = $val;
			if ($key === $this->config('websmsd','key_body')) $message['body'] = $val;
		});
		if (empty($message['to']) || empty($message['from']) || empty($message['body'])) {
			trigger_error(sprintf("Did not get all required POST data (%s) message (%s)",
				json_encode($post_data),json_encode($message)), E_USER_WARNING);
		}
		return $message;
	}
	/*--------------------------------------------------------------------------
	Initially assume 'REMOTE_ADDR' is the original IP of the HTTP client. But if
	it is a 'prox_addr' then trust that 'prox_header' hold the real IP of the
	client instead.
	@return string $remt_addr
	*/
	private function remt_addr() {
		$remt_addr = $_SERVER['REMOTE_ADDR'];
		$prox_addr = $this->config('websmsd','prox_addr');
		$prox_header = $this->config('websmsd','prox_header');
		if (!empty($prox_addr) && !empty($prox_header) &&
			$this->match_cidr($remt_addr,$prox_addr)) {
			$header_addr = @$_SERVER[$prox_header];
			if (isset($header_addr)) {
				$remt_addr = $header_addr;
			} else {
				trigger_error(
					sprintf("REMOTE_ADDR (%s) matches proxy (%s) but there is no header (%s)",
					$remt_addr, $prox_addr, $prox_header), E_USER_WARNING);
			}
		}
		return $remt_addr;
	}
	/*--------------------------------------------------------------------------
	Compare each uri (comma separated) with the REQUEST_URI
	@param string $csv_uris        uris to test eg, /index.htm,/index.html
	@return Boolean true if $saddr is permitted otherwise false
	*/
	private function match_uri($csv_uris) {
		$request_uri = $_SERVER['REQUEST_URI'];
		$match_uris = explode(',',$csv_uris);
		foreach ($match_uris as $match_uri) {
			if(strcmp($match_uri,$request_uri) === 0) return true;
		}
		return false;
	}
	/*--------------------------------------------------------------------------
	@param string $ip        addr to test eg, 216.245.217.2
	@param string $csvcidrs  comma separated list of CIDR ranges
	                          eg, 185.45.152.42,3.104.90.0/24,3.1.77.0/24
	@return Boolean true if $ip matches any range in $csvcidrs
	*/
	public function match_cidr($ip,$csvcidrs) {
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
	Receive an additional header "Signature",
	$signatureTest = base64_encode(hash_hmac('sha1', $_POST['result'], API_SECRET));
	@param array $post HTTP POST
	@return Boolean true if HTTP request was verified
	*/
	private function verify($post) {
		$auth_method = $this->config('websmsd','auth_method');
		$auth_secret = $this->config('websmsd','auth_secret');
		switch ($auth_method) {
		case '':
			return true;
			break;
		case 'zadarma':
			$result = $post['result'];
			$sign = $post['Signature'];
			$sign_expected = base64_encode(hash_hmac('sha1', $result, $auth_secret));
			return ($sign === $sign_expected);
			break;
		default:
			trigger_error("Unknown method (auth_method=$auth_method)", E_USER_WARNING);
			return true;
			break;
		}
	}
	/*--------------------------------------------------------------------------
	Parse arguments.
	We require 3 arguments; to, from, body.
	OR 4 arguments; to, from, body, index.
	@param  array $args arguments
	@return array $query_data or false
	*/
	private function query_data($args) {
		switch (count($args)) {
		case 4:
			$index = $args[4];
			if(strlen($index) > 0) $this->index = $index;
		case 3:
			$query_data = [
			$this->config('websms','key_to')   => $args[1],
			$this->config('websms','key_from') => $args[2],
			$this->config('websms','key_body') => $args[3],
			];
			return $query_data;
		default:
			$strings = implode(',',$args);
			trigger_error("We did not get exactly 3 or 4 arguments; to, from, body [index] ($strings)", E_USER_WARNING);
			return false;
		}
	}
	/*--------------------------------------------------------------------------
	Sanitize telephone numbers.
	@param array $query_data
	@void
	*/
	private function val_numform(&$query_data) {
		$key_to = $this->config('websms','key_to');
		$key_from = $this->config('websms','key_from');
		switch (strtolower($this->config('websms','val_numform'))) {
		case 'no+':
		case 'e164':
		case 'e.164':
			$query_data[$key_to] = preg_replace(['/^[+]/','/^00/'], '',$query_data[$key_to]);
			$query_data[$key_from] = preg_replace(['/^[+]/','/^00/'], '',$query_data[$key_from]);
			break;
		case 'e.123':
		default:
		}
	}
	/*--------------------------------------------------------------------------
	Add key-value pairs in $query_data
	@param array $query_data
	@void
	*/
	private function val_static(&$query_data) {
		$val_static = $this->config('websms','val_static');
		if (!empty($val_static)) {
			$static_keyvals = explode(',',$val_static);
			foreach ($static_keyvals as $static_keyval) {
				if (strpos($static_keyval, '=') !== false) {
					list($key, $val) = explode('=',$static_keyval);
					$query_data[$key] = $val;
				}
			}
		}
	}
	/*--------------------------------------------------------------------------
	Sanitize body, since some API only accept Unicode up to xFFFF BMP (UCS-2).
	ucs-2:
		Replace characters in SMP with the replacement character U+FFFD, when needed.
	key=val:
		If There is SMP characters add the provided key:val
	otherwise:
		Do nothing.
	@param array $query_data
	@void
	*/
	private function val_unicode(&$query_data) {
		$val_unicode = $this->config('websms','val_unicode');
		$key_body = $this->config('websms','key_body');
		if (strpos($val_unicode, '=') !== false) {
			list($key_code, $val_code) = explode('=',$val_unicode);
			$val_unicode = 'key=val';
		}
		switch (strtolower($val_unicode)) {
		case 'ucs-2':
			$query_data[$key_body] = preg_replace('/[\x{10000}-\x{10FFFF}]/u',"\u{FFFD}",$query_data[$key_body]);
			break;
		case 'key=val':
			if (!empty(preg_grep("/[\x{10000}-\x{10FFFF}]/u",[$query_data[$key_body]])))
				$query_data[$key_code] = $val_code;
			break;
		case 'utf-8':
		default:
		}
	}
	/*--------------------------------------------------------------------------
	Init and setup curl for a POST query.
	@return void
	*/
	private function curl_init() {
		$url = $this->config('websms','url_host').$this->config('websms','url_path');
		$this->curl = curl_init($url);
		curl_setopt($this->curl, CURLOPT_RETURNTRANSFER, true);
		curl_setopt($this->curl, CURLOPT_POST, true);
		curl_setopt($this->curl, CURLOPT_SSL_VERIFYPEER, false);
		curl_setopt($this->curl, CURLOPT_SSL_VERIFYHOST, false);
	}
	/*--------------------------------------------------------------------------
	Generates a URL-encoded query string from the $query_data array provided,
	and pass it on to curl.
	@param  array  $query_data
	@return string $query_string
	*/
	private function curl_data($query_data) {
		ksort($query_data);
		$this->debug($query_data);
		$query_string = http_build_query($query_data);
		curl_setopt($this->curl, CURLOPT_POSTFIELDS, $query_string);
		return $query_string;
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
	@param  array  $query_data
	@return void
	*/
	private function curl_auth(&$query_data) {
		$auth_method = $this->config('websms','auth_method');
		$auth_user = $this->config('websms','auth_user');
		$auth_secret = $this->config('websms','auth_secret');
		$key_user = $this->config('websms','key_user');
		$key_secret = $this->config('websms','key_secret');
		$url_path = $this->config('websms','url_path');
		switch ($auth_method) {
		case 'none':
			break;
		case 'plain':
			if (!empty($key_user) && !empty($auth_user) && !empty($key_secret) && !empty($auth_secret)) {
				$query_data[$key_user] = $auth_user;
				$query_data[$key_secret] = $auth_secret;
			} else trigger_error(
				sprintf("Authorization method (%s) but not all needed parameters are defined",$auth_method), E_USER_WARNING);
			break;
		case 'basic':
			if (!empty($auth_user) && !empty($auth_secret)) {
				curl_setopt($this->curl, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
				curl_setopt($this->curl, CURLOPT_USERPWD, "$auth_user:$auth_secret");
			} else trigger_error(
				sprintf("Authorization method (%s) but not all needed parameters are defined",$auth_method), E_USER_WARNING);
			break;
		case 'zadarma':
			if (!empty($auth_user) && !empty($auth_secret)) {
				$query_string = curl_data($query_data);
				$signature = base64_encode(hash_hmac('sha1', $url_path .
					$query_string . md5($query_string), $auth_secret));
				curl_setopt($this->curl, CURLOPT_HTTPHEADER,
					array('Authorization: ' . $auth_user . ':' . $signature));
			} else trigger_error(
				sprintf("Authorization method (%s) but not all needed parameters are defined",$auth_method), E_USER_WARNING);
			break;
		default: trigger_error(
			sprintf("Unknown authorization method (auth_method=%s)",$auth_method), E_USER_WARNING);
		}
	}
	/*--------------------------------------------------------------------------
	Send POST query, read response and close.
	@return  array  $resp_data
	*/
	private function curl_send() {
		$resp_json = curl_exec($this->curl);
		if (!empty($resp_json)) {
			$resp_data = json_decode($resp_json,true);
		} else {
			$resp_data = false;
			trigger_error("Curl error: ".curl_error($this->curl));
		}
		$curl_info = curl_getinfo($this->curl);
		curl_close($this->curl);
		//$this->debug($curl_info);
		$this->debug($resp_json);
		return $resp_data;
	}
	/*--------------------------------------------------------------------------
	Check response and set exit code accordingly.
	the response check is controlled by the variable resp_check. It can be:
	= null\""; No check, we will always exit with status true
	= "key=value" If value of key in response is equal to value exit true otherwise false
	= "/pattern/"; If pattern matches response exit with status true otherwise false
	{
		"status":"success",
		"messages":1,
		"cost":0.24,
		"currency":"USD"
	}
	@param  array $resp_data
	@return Boolean true if resp_check matches, otherwize false
	*/
	private function resp_check($resp_data) {
		$resp_check = $this->config('websms','resp_check');
		$url_host = $this->config('websms','url_host');
		$auth_user = $this->config('websms','auth_user');
		if (empty($resp_data)) return false;
		if (!empty($resp_check)) {
			if (strpos($resp_check, '=') !== false) { // "key=value"
				list($test_key, $test_value) = explode('=',$resp_check);
				// test hierarchically
				array_walk_recursive($resp_data, function($val,$key) use (&$resp_value,$test_key) {
					if ($key === $test_key) $resp_value = $val;
				});
				if ($resp_value !== $test_value) {
					trigger_error(sprintf("Called (%s) but return did not match (%s = %s != %s)",
						$url_host,$test_key,$resp_value,$test_value), E_USER_WARNING);
					trigger_error(sprintf("Response was: %s",json_encode($resp_data)));
					return false;
				}
			} else { // "/pattern/"
				if (!preg_match($resp_check,$resp_data)) {
					trigger_error(sprintf("Called (%s) but return did not match (%s != %s)",
						$url_host,$resp_data,$resp_check), E_USER_WARNING);
					trigger_error(sprintf("Response was: %s",json_encode($resp_data)));
					return false;
				}
			}
			trigger_error("Outbound SMS sent (host=$url_host,user=$auth_user) successfully", E_USER_NOTICE);
		} else {
			trigger_error("Outbound SMS sent (host=$url_host,user=$auth_user)", E_USER_NOTICE);
		}
		return true;
	}
	/*--------------------------------------------------------------------------
	Get configuration value, if $this->index is set pick that element in array
	@param  mixed   $section
	@param  mixed   $key
	@param  Boolean $allow_array
	@return mixed   $value or $array_of_values if $allow_array = true
	*/
	public function config($section, $key, $allow_array = false) {
		$value = $this->config[$section][$key];
		$indices = $this->config_indices($section);
		if (is_array($value)) {
			if (isset($this->index)) {
				if (array_key_exists($this->index,$value)) {
					$value = $value[$this->index];
				} else {
					$value = self::DEFAULT_CONF_VALS[$section][$key];
					if (!in_array($this->index,$indices)) trigger_error(
						sprintf("Config ([%s] %s [%s]) missing; using default",
						$section,$key,$this->index), E_USER_NOTICE);
				}
			} else {
				if ($allow_array !== true) {
					trigger_error(
						sprintf("Config ([%s] %s) is array but index is not provided",
						$section,$key), E_USER_ERROR);
					$value = null;
				}
			}
		}
		return $value;
	}
	/*--------------------------------------------------------------------------
	*/
	public function config_indices($section) {
		$indices = [];
		array_walk($this->config[$section], function($val,$key) use (&$indices) {
			if(is_array($val))
				foreach($val as $key => $value)
					if(!in_array($key, $indices))
						$indices[] = $key;
		});
		return $indices;
	}
	/*--------------------------------------------------------------------------
	Print variable if $debug or $this->debug is true
	@param  mixed   $var
	@param  boolean $debug
	@return void
	*/
	public function debug($var, $debug = false) {
		if($debug || $this->debug) {
			var_dump($var);
		}
	}
}
?>
