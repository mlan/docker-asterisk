<?php
/*------------------------------------------------------------------------------
 The Astqueue class provides functions for creating Astrisk call files

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
		'priority'        => 1,
		];
	private $config;
	private $name;
	private $call;
	private $mess;
	public $debug = false;
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
		$this->mkdir($this->config['astqueue']['outgoingdir']);
		$this->mkdir($this->config['astqueue']['stagingdir']);
	}
	/*--------------------------------------------------------------------------
	@param string $dir full path to direcory
	@return void
	*/
	private function mkdir($dir) {
		if (!is_dir($dir) && !mkdir($dir))
			trigger_error("unable create directory ($dir)", E_USER_WARNING);
	}
	/*--------------------------------------------------------------------------
	Generate call file with message
	@param array $message, eg ['to'=>'+12025550160','from'=>'+12025550125','body'=>'Hello']
		$message['to'] : The SMS destination
		$message['from'] : The SMS originator
		$message['body'] : The SMS message
	@return boolean true if sucessful otherwise false
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
	/*--------------------------------------------------------------------------
	Generate call file with call
	@param string $exten callee phone number, eg '+12025550160'
	@param string $callid caller phone number, eg '+12025550125'
	@return boolean true if sucessful otherwise false
	*/
	public function call($exten,$callid) {
		if (isset($exten) && isset($callid)) {
			$this->gen_name($exten);
			$this->gen_call($exten,$callid);
			return $this->write();
		} else {
			return false;
		}
	}
	/*--------------------------------------------------------------------------
	Create new call file in the staging directory.
	Using staging directory to avoid Asterisk reading an unfinished file.
	Move the call file to the outgoing directory, so that Asterisk pick it up.
	@return boolean true if sucessful otherwise false
	*/
	private function write() {
		$lines  = $this->gen_callline($this->call,'call');
		$lines .= $this->gen_callline($this->mess,'mess');
		$stagingfile  = $this->config['astqueue']['stagingdir'].'/'.$this->name;
		$outgoingfile = $this->config['astqueue']['outgoingdir'].'/'.$this->name;
		if (file_put_contents($stagingfile,$lines) === false) {
			trigger_error("unable to open call file ($stagingfile)", E_USER_WARNING);
			return false;
		}
		if (rename($stagingfile, $outgoingfile) === false) {
			trigger_error("unable to move file ($stagingfile) to file ($outgoingfile)", E_USER_WARNING);
			return false;
		}
		return true;
	}
	/*--------------------------------------------------------------------------
	NOT WORKING DUE TO PHP NOT HAVING ROOT ASSESS
	*/
	private function chmod() {
		$outgoingfile = $this->config['astqueue']['outgoingdir'].'/'.$this->name;
		if (!empty($this->config['astqueue']['filemode'])) {
			chmod($outgoingfile, octdec($this->config['astqueue']['filemode']));
		}
		if (!empty($this->config['astqueue']['fileowner'])) {
			chmod($outgoingfile, chown($this->config['astqueue']['fileowner']));
		}
	}
	/*--------------------------------------------------------------------------
	Generate call file name.
	Format: <EXTEN>.<seconds since the Unix Epoch>.<3 digit random>.call
	@param string $exten callee phone number, eg '+12025550160'
	@return void
	*/
	private function gen_name($exten) {
		$this->name = sprintf("%s.%d.%03d.call",$exten,time(),rand(0,999));
	}
	/*--------------------------------------------------------------------------
	Syntax of the Asterisk call file:
		Channel: The channel to use for the new call
		CallerID: The caller id to use
		Maxretries: Number of retries before failing
		RetryTime: How many seconds to wait before retry
		Context: The context in the dialplan
		Extension: The extension in the specified context
		Priority: The priority of the specified extension
		Setvar: MESSAGE(to)= The SMS destination
		Setvar: MESSAGE(from)= The SMS originator
		Setvar: MESSAGE(body)= The SMS message
	@param string $exten callee phone number, eg '+12025550160'
	@param string $callid caller phone number, eg '+12025550125'
	@return void
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
		unset($value);
		$this->debug($this->call);
	}
	/*--------------------------------------------------------------------------
	Assign class variable mess.
	@param array $message, eg ['to'=>'+12025550160','from'=>'+12025550125','body'=>'Hello']
		$message['to'] : The SMS destination
		$message['from'] : The SMS originator
		$message['body'] : The SMS message
	@return boolean true if sucessful otherwise false
	*/
	private function gen_mess($message) {
		if (isset($message['to']) && isset($message['from']) && isset($message['body'])) {
			$this->mess = $message;
			$this->debug($this->mess);
			return true;
		} else {
			return false;
		}
	}
	/*--------------------------------------------------------------------------
	Generate channel string
	@param string $exten callee phone number, eg '+12025550160'
	@return string channel
	*/
	private function gen_channel($exten) {
		return sprintf('Local/%s@%s',$exten,$this->config['astqueue']['channel_context']);
	}
	/*--------------------------------------------------------------------------
	Generate callid string
	@param string $number caller phone number, eg '+12025550183'
	@param string $display caller display, eg 'Jane Doe'
	@return string callid, eg '"Jane Doe" <+12025550183>'
	*/
	private function gen_callid($number,$display = '') {
		return sprintf('"%s" <%s>',$display,$number);
	}
	/*--------------------------------------------------------------------------
	Generate call file line format
	@param  array  $array, eg ['body'=>'hello']
	@param  string $type, eg 'mess'
	TODO @return string format string, eg 'setvar: MESSAGE(body)="Hello"
	*/
	public function gen_callline($array,$type = '') {
		$return = null;
		$format = $this->gen_callformat($type);
		foreach ($array as $key => $value) {
			if (isset($value)) {
				$return .= sprintf($format,$key,$this->escapespecial($value));
			}
		}
		return $return;
	}
	/*--------------------------------------------------------------------------
	Generate call file line format
	@param string $type, eg 'mess'
	TODO @return string format string, eg 'setvar: MESSAGE(%s)="%s"'
	*/
	private function gen_callformat($type) {
		switch ($type) {
			case 'mess':
				return 'setvar: MESSAGE(%s)=%s'.PHP_EOL;
			case 'setvar':
				return 'setvar: %s=%s'.PHP_EOL;
			case 'call':
			default:
				return '%s: %s'.PHP_EOL;
		}
	}
	/*--------------------------------------------------------------------------
	In an Astrisk dialplan (extensions.conf) the following characters must be
	escaped when used in a variable: []$"\
	replace LF/CR with VT or FF
	@param string $string to escape
	@return string escaped string
	*/
	private function escapespecial($string) {
		$patterns = ['/\R/','/([;])/'];
		$replacements = ["\u{000c}","\\\\\\1"];
		return preg_replace($patterns, $replacements, $string);
#		return preg_replace('/\R/','\x000A',$string);
#		return preg_replace('/([\[\]$"\\\\])/',"\\\\\\1",$string);
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
