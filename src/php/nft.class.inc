<?php
/**
* The Nft class provides functions for banning and unbanning source ip addessed
* using nftables.
*/

class Nft {
	private const DEFAULT_CONF_FILE = '/etc/asterisk/nft.conf';
	private const DEFAULT_CONF_VALS = [
		'cmd'   => 'nft',
		'sub'   => null,
		'table' => 'inet',
		'type'  => 'filter',
		'chain' => 'input',
		'pre'   => null,
		'val'   => null,
		'pst'   => null,
		'opt'   => null
	];
	public $config;
	private $ruleHandles = [];
	public $debug = false;
	public function __construct($config = null, array $optconfig = []) {
		if (is_string($config) !== true) {
			$config = self::DEFAULT_CONF_FILE;
		}
		$this->config['nft'] = self::DEFAULT_CONF_VALS;
		if (file_exists($config) === true) {
			$config_ini = parse_ini_file($config,true);
			$this->config['nft'] = array_merge($this->config['nft'],$config_ini['nft']);
		}
		foreach ($optconfig as $var => $val) {
			$this->config['nft'][$var] = $val;
		}
	}
	public function ban($ip) {
		$args = ['sub'=>'add rule','pre'=>'ip saddr','val'=>$ip,
			'pst'=>'drop\;','opt'=>'-e -a'];
		$ret = $this->exec($args);
		if (!empty($ret)) {
			$this->setHandle($ip,$ret);
		}
	}
	public function unban($ip) {
		$handle = $this->getHandle($ip);
		if ($handle !== false) {
			$args = ['sub'=>'delete rule','pre'=>'handle','val'=>$handle];
			if ($this->exec($args) !== false) {
				$this->rmHandle($ip);
			}
		}
	}
	public function reset() {
		$args = ['sub'=>'flush chain'];
		$this->exec($args);
	}
	public function list() {
		$args = ['sub'=>'list chain'];
		$ret = $this->exec($args);
		$this->debug($ret,true);
	}
	private function exec($args) {
		$exec_array = array_merge($this->config['nft'],$args);
		$exec_string = implode(' ',$exec_array);
		exec($exec_string,$return,$status);
		if ($status === 0) {
			return $return;
		} else {
			$this->log('('.$exec_array['sub'].')', $return, E_USER_WARNING);
			$this->debug($exec_string);
			return false;
		}
	}
	private function setHandle($ip,$return) {
		$ret_split = preg_split('/ # handle /',$return[0]);
		if (!empty($ret_split[1])) {
			$handle = $ret_split[1];
			$this->ruleHandles[$ip] = $handle;
			return $handle;
		} else {
			$this->log('did not return handle ('.$ip.')', $return, E_USER_WARNING);
			$this->debug($ret_split);
			return false;
		}
	}
	private function getHandle($ip) {
		if (isset($this->ruleHandles[$ip])) {
			return $this->ruleHandles[$ip];
		} else {
			return false;
		}
	}
	private function rmHandle($ip) {
		if (isset($this->ruleHandles[$ip])) {
			$this->ruleHandles = array_diff_key($this->ruleHandles,[$ip=>0]);
		}
	}
	
	public function log($message, $error = [], $level = E_USER_NOTICE) {
		$message = 'nftables '.$message;
		if (isset($error[0])) {
			$message = $message.' error: '.$error[0];
		}
		trigger_error($message, $level);
	}
	public function debug($data, $debug = false) {
		if($debug || $this->debug)
			var_dump($data);
	}
}
?>

