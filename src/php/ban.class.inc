<?php
/**
* The Ban class provides functions for monitoring abuse and if it persists ban
* offending ip. Within a time period, `period=300` s a maximum of `count=3`
* abuses are allowd per remote ip before they are banned for a
* `prune_interval=1800` time interval.
*/
class Ban {
	const DEFAULT_CONF_FILE = '/etc/asterisk/autoban.conf';
	const DEFAULT_CONF_VALS = [
		'enabled'        => true,
		'count'          => 3,
		'period'         => 300,
		'prune_interval' => 1800
	];
	public $config;
	private $last_time;
	private $ip_abuse_count = [];
	private $ip_ban_time = [];
	public $debug = false;
	public function __construct($config = null, array $optconfig = []) {
		if (is_string($config) !== true) {
			$config = self::DEFAULT_CONF_FILE;
		}
		$this->config['autoban'] = self::DEFAULT_CONF_VALS;
		if (file_exists($config) === true) {
			$config_ini = parse_ini_file($config,true);
			$this->config['autoban'] = array_merge($this->config['autoban'],$config_ini['autoban']);
		} else {
			$this->config['autoban']['enabled'] = false;
		}
		foreach ($optconfig as $var => $val) {
			$this->config['autoban'][$var] = $val;
		}
		$this->reset();
	}
	public function abuse($ip) {
		if (isset($this->ip_ban_time[$ip])) {
			$this->ip_ban_time[$ip] = time();
			$this->log("Extend ban for $ip");
			$this->debug($this->ip_ban_time);
		} else {
			if (isset($this->ip_abuse_count[$ip])) {
				$this->ip_abuse_count[$ip]++;
			} else {
				$this->ip_abuse_count[$ip] = 1;
			}
			$this->log("Abuse log $ip=".number_format($this->ip_abuse_count[$ip],2));
			if ($this->ip_abuse_count[$ip] >= $this->config['autoban']['count']) {
				$this->ip_ban_time[$ip] = time();
				$this->log("Will ban $ip",E_USER_WARNING);
				return $ip;
			}
		}
		$this->debug($this->ip_abuse_count);
		return false;
	}
	public function prune() {
		$periods_elapsed = (time() - $this->last_time)/$this->config['autoban']['period'];
		foreach ($this->ip_abuse_count as &$count)
			$count = max($count - $periods_elapsed,0);
		unset($count);
		$this->ip_abuse_count = array_filter($this->ip_abuse_count, function($c) { return $c > 0; } );
		$this->debug($this->ip_abuse_count);
		$this->last_time = time();
		if ($this->config['autoban']['prune_interval'] < 0) return false;
		$past_time = time() - $this->config['autoban']['prune_interval'];
		$ip_to_unban = array_filter($this->ip_ban_time, function($t) use ($past_time) { return $t < $past_time; } );
		if (empty($ip_to_unban)) {
			//$this->log("Aging abuse and bans $periods_elapsed s");
			return false;
		} else {
			$this->ip_ban_time = array_diff_key($this->ip_ban_time,$ip_to_unban);
			$ips = array_keys($ip_to_unban);
			$this->log("Will unban ".implode(",",$ips),E_USER_WARNING);
			return $ips;
		}
	}
	public function reset() {
		$this->last_time = time();
		$this->ip_abuse_count = [];
		$this->ip_ban_time = [];
	}
	public function log($message, $level = E_USER_NOTICE) {
		$nr_abuse = count($this->ip_abuse_count);
		$nr_ban = count($this->ip_ban_time);
		$last_abuse = key(array_slice($this->ip_abuse_count,-1,1,true));
		$last_ban = key(array_slice($this->ip_ban_time,-1,1,true));
		$db_state = " (abusers=".$nr_abuse.", alast=".$last_abuse.", banned=".$nr_ban.", blast=".$last_ban.")";
		trigger_error($message.$db_state, $level);
	}
	public function debug($data, $debug = false) {
		if($debug || $this->debug)
			var_dump($data);
	}
}
?>
