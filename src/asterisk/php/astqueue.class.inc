<?php
/**
* The Astqueue class provides functions for creating Astrisk call files
*
*/
class Astqueue {
	const DEFAULT_CONF_FILE = '/etc/asterisk/astqueue.conf';
	const DEFAULT_CONF_VALS = [
		'outgoingdir'     => '/var/spool/asterisk/outgoing',
		'stagingdir'      => '/var/spool/asterisk/staging',
		'filemode'        => null,
		'fileowner'       => null,
		'waittime'        => 45,
		'maxretries'      => 0,
		'retrytime'       => 300,
		'archive'         => 'no',
		'channel_context' => 'default',
		'context'         => 'default',
		'priority'        => 1
		];
	private $config;
	private $name;
	private $call;
	private $mess;
	public $debug = true;
	public function __construct($config = null, array $optconfig = []) {
		if (is_string($config) !== true) {
			$config = self::DEFAULT_CONF_FILE;
		}
		$this->config['astqueue'] = self::DEFAULT_CONF_VALS;
		if (file_exists($config) === true) {
			$config_ini = parse_ini_file($config,true);
			$this->config['astqueue'] = array_merge($this->config['astqueue'],
				$config_ini['astqueue']);
		}
		foreach ($optconfig as $var => $val) {
			$this->config['astqueue'][$var] = $val;
		}
	}
	/**
	* $message['to'] : The SMS destination
	* $message['from'] : The SMS originator
	* $message['body'] : The SMS message
	*/
	public function text($message) {
		if (isset($message['to']) && isset($message['from']) && isset($message['body'])) {
			$exten = $message['to'];
			$callid = $message['from'];
			$this->gen_name($exten);
			$this->gen_call($exten,$callid);
			$this->gen_mess($message);
			return $this->write();
		} else {
			return false;
		}
	}
	public function call($exten,$callid) {
		if (isset($exten) && isset($callid)) {
			$this->gen_name($exten);
			$this->gen_call($exten,$callid);
			return $this->write();
		} else {
			return false;
		}
	}
	/**
	* Create new call file in the staging directory.
	* Using staging directory to avoid Asterisk reading an unfinished file.
	* Move the call file to the outgoing directory, so that Asterisk pick it up.
	*/
	private function write() {
		$strings  = $this->strings($this->call,'call');
		$strings .= $this->strings($this->mess,'mess');
		$stagingfile  = $this->config['astqueue']['stagingdir'].'/'.$this->name;
		$outgoingfile = $this->config['astqueue']['outgoingdir'].'/'.$this->name;
		if (file_put_contents($stagingfile,$strings) === false) {
			trigger_error("unable to open call file ($stagingfile)", E_USER_WARNING);
			return false;
		}
		if (rename($stagingfile, $outgoingfile) === false) {
			trigger_error("unable to move file ($stagingfile) to file ($outgoingfile)", E_USER_WARNING);
			return false;
		}
		return true;
	}
	/* NOT WORKING DUE TO PHP NOT HAVING ROOT ASSESS*/
	private function chmod() {
		$outgoingfile = $this->config['astqueue']['outgoingdir'].'/'.$this->name;
		if (!empty($this->config['astqueue']['filemode'])) {
			chmod($outgoingfile, octdec($this->config['astqueue']['filemode']));
		}
		if (!empty($this->config['astqueue']['fileowner'])) {
			chmod($outgoingfile, chown($this->config['astqueue']['fileowner']));
		}
	}
	/**
	* Generate call file name.
	* Format: <EXTEN>.<seconds since the Unix Epoch>.<3 digit random>.call
	*/
	private function gen_name($exten) {
		$this->name = sprintf("%s.%d.%03d.call",$exten,time(),rand(0,999));
	}
	/**
	* Syntax of the Asterisk call file:
	* Channel: The channel to use for the new call
	* CallerID: The caller id to use
	* Maxretries: Number of retries before failing
	* RetryTime: How many seconds to wait before retry
	* Context: The context in the dialplan
	* Extension: The extension in the specified context
	* Priority: The priority of the specified extension
	* Setvar: MESSAGE(to)= The SMS destination
	* Setvar: MESSAGE(from)= The SMS originator
	* Setvar: MESSAGE(body)= The SMS message
	*/
	private function gen_call($exten,$callid) {
		$this->call = [
		'Channel'     => $this->gen_channel($exten),
		'CallerID'    => $this->gen_callid($callid),
		'WaitTime'    => null,
		'MaxRetries'  => null,
		'RetryTime'   => null,
		'Account'     => null,
		'Application' => null,
		'Data'        => null,
		'Context'     => null,
		'Extension'   => $exten,
		'Priority'    => null,
		'Archive'     => null
		];
		foreach ($this->call as $key => &$value) {
			$key_lower = strtolower($key);
			if(empty($value) && array_key_exists($key_lower,$this->config['astqueue'])) {
				$value = $this->config['astqueue'][$key_lower];
			}
		}
		$this->debug($this->call);
		unset($value);
	}
	private function gen_mess($message) {
		if (isset($message['to']) && isset($message['from']) && isset($message['body'])) {
			$this->mess = $message;
			$this->debug($this->mess);
			return true;
		} else {
			return false;
		}
	}
	private function gen_channel($exten) {
		return sprintf('Local/%s@%s',$exten,$this->config['astqueue']['channel_context']);
	}
	private function gen_callid($num,$name = '') {
		return sprintf('"%s" <%s>',$name,$num);
	}
	private function strings($array,$type = '') {
		$strings = null;
		$format = $this->format($type);
		foreach ($array as $key => $value) {
			if (isset($value)) {
				$strings .= sprintf("$format",$key,$value);
			}
		}
		return $strings;
	}
	private function format($type) {
		switch ($type) {
			case 'mess':
				$format = "setvar: MESSAGE(%s)=%s\n";
				break;
			case 'setvar':
				$format = "setvar: %s=%s\n";
				break;
			case 'call':
			default:
				$format = "%s: %s\n";
				break;
		}
		return $format;
	}
	public function debug($data, $debug = false) {
		if($debug || $this->debug) {
			var_dump($data);
		}
	}
}
?>
