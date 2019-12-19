<?php
/*------------------------------------------------------------------------------
 The Astqueue class provides functions for creating Astrisk call files

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
		'message_encode'  => 'rfc3986',
		];
	const CALLFILE_SYNTAX = [
		'Channel'         => 'channel',
		'CallerID'        => 'callid',
		'WaitTime'        => 'config',
		'MaxRetries'      => 'config',
		'RetryTime'       => 'config',
		'Account'         => 'config',
		'Application'     => 'config',
		'Data'            => 'config',
		'Context'         => 'config',
		'Extension'       => 'to',
		'Priority'        => 'config',
		'Archive'         => 'config',
		];
	const CALLFILE_SETVAR = [
		'MESSAGE(to)'     => 'to',
		'MESSAGE(from)'   => 'from',
		'MESSAGE(body)'   => 'body',
		'MESSAGE_ENCODE'  => 'config',
		];
	private $config;
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
	Generate call file with message
	@param  array   $data, eg ['to'=>'+1...60','from'=>'+1...25','body'=>'Hello!']
		$data['to'] : The SMS destination
		$data['from'] : The SMS originator
		$data['body'] : The SMS message
	@return boolean true if sucessful otherwise false
	*/
	public function text($data) {
		if (isset($data['to']) && isset($data['from']) && isset($data['body'])) {
			$lines = $this->gen_lines(Self::CALLFILE_SYNTAX,$data);
			$lines .= $this->gen_lines(Self::CALLFILE_SETVAR,$data,true);
			$basename = $this->gen_basename($data);
			return $this->write($basename, $lines);
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
			$data = ['to'=>$exten,'from'=>$callid];
			$lines = $this->gen_lines(Self::CALLFILE_SYNTAX,$message);
			$basename = $this->gen_basename($data);
			return $this->write($basename, $lines);
		} else {
			return false;
		}
	}
	/*--------------------------------------------------------------------------
	@param  array   $syntax, eg ['Channel'=>'channel','CallerID'=>'callid',...]
	@param  array   $data, eg ['to'=>'+1...60','from'=>'+1...25','body'=>'Hello!']
	@param  boolean $setvar use setvar syntax if true
	@return string  lines
	*/
	private function gen_lines($syntax,$data,$setvar = false) {
		$return = null;
		$to = $data['to'];
		$from = $data['from'];
		$body = $data['body'];
		foreach ($syntax as $key => $type) {
			switch ($type) {
			case 'to':
				$return .= $this->gen_line($key,$to,$setvar);
				break;
			case 'from':
				$return .= $this->gen_line($key,$from,$setvar);
				break;
			case 'body':
				$return .= $this->gen_line($key,$this->gen_body($body),true);
				break;
			case 'channel':
				$return .= $this->gen_line($key,$this->gen_channel($to));
				break;
			case 'callid':
				$return .= $this->gen_line($key,$this->gen_callid($from));
				break;
			case 'config':
				$return .= $this->gen_line($key,$this->gen_config($key),$setvar);
				break;
			}
		}
		return $return;
	}
	/*--------------------------------------------------------------------------
	@param  string  $key, eg 'archive'
	@param  string  $value, eg 'yes'
	@param  boolean $setvar use setvar syntax if true
	@return string  or null, eg 'archive: yes'
	*/
	private function gen_line($key,$value,$setvar = false) {
		if (!isset($key) || !isset($value)) return null;
		if ($setvar) {
			return sprintf('setvar: %s=%s', $key, $value).PHP_EOL;
		} else {
			return sprintf('%s: %s', $key, $value).PHP_EOL;
		}
	}
	/*--------------------------------------------------------------------------
	Get config value
	@param string $key, eg 'waittime'
	@return mixed config value, eg 45
	*/
	public function gen_config($key) {
		$key_lower = strtolower($key);
		if(array_key_exists($key_lower,$this->config['astqueue'])) {
			return $this->config['astqueue'][$key_lower];
		} else {
			return null;
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
	Generate call file basename.
	Format: <EXTEN>.<seconds since the Unix Epoch>.<3 digit random>.call
	@param  array $data['to'] callee phone number, eg '+12025550160'
	@return void
	*/
	private function gen_basename($data) {
		return sprintf("%s.%d.%03d.call",$data['to'],time(),rand(0,999));
	}
	/*--------------------------------------------------------------------------
	The message cannot span multiple lines in an Astrisk call file. To work
	arround that we encode the message (RFC3986, which superseeds RFC2396).
	@param string $string to escape
	@return string escaped string
	*/
	private function gen_body($string) {
		if ($this->config['astqueue']['message_encode'] === 'rfc3986') {
			return rawurlencode($string);
		} else {
			return $string;
		}
	}
	/*--------------------------------------------------------------------------
	Create new call file in the staging directory.
	Using staging directory to avoid Asterisk reading an unfinished file.
	Move the call file to the outgoing directory, so that Asterisk pick it up.
	@param string $basename
	@param string $lines file contents
	@return boolean true if sucessful otherwise false
	*/
	private function write($basename, $lines) {
		$stagingfile  = $this->config['astqueue']['stagingdir'].'/'.$basename;
		$outgoingfile = $this->config['astqueue']['outgoingdir'].'/'.$basename;
		if (file_put_contents($stagingfile,$lines) === false) {
			trigger_error("unable to open call file ($stagingfile)", E_USER_WARNING);
			return false;
		}
		if (rename($stagingfile, $outgoingfile) === false) {
			trigger_error("unable to move file ($stagingfile) to file ($outgoingfile)",
				E_USER_WARNING);
			return false;
		}
		return true;
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
