<?php
/*------------------------------------------------------------------------------
 The Autoban class provides functions for watching and jailing IP addresses
 using nftables.
 The state information is represented by timeouts in NFT, so that it can be
 recovered after a system reboot.

 The NFT set 'watch' hold a watch list of IPs. The watch count is represented by
 the number of seconds beyond 'watchtime' in the timeout

 The NFT set 'jail' hold IPs that whose packets will be dropped by NFT. When
 the timeout completes the IP is released.

 The NFT set 'parole' hold a list of IPs which was recently released. If watched
 during the parole time the IP is immediately jailed again and now 'repeatmult'
 longer.
*/
require_once 'nft.class.inc';

class Autoban extends Nft {
	private const DEFAULT_CONF_FILE = '/etc/asterisk/autoban.conf';
	public const DEFAULT_CONF_VALS = [
		'enabled'    => true,
		'maxcount'   => 10,
		'watchtime'  => '20m',
		'jailtime'   => '20m',
		'repeatmult' => 6
	];
	private const TIME_MAXSEC = 99999999;
	public $config;
#	private $timeout = [ 'watch' => [], 'jail' => [], 'parole' => [] ];
	public $debug = false;
	public function __construct($config = null, array $optconfig = []) {
		parent::__construct();
		if (is_string($config) !== true) {
			$config = self::DEFAULT_CONF_FILE;
		}
		$this->config['autoban'] = self::DEFAULT_CONF_VALS;
		if (file_exists($config) === true) {
			$config_ini = parse_ini_file($config,true);
			if (!empty($config_ini['autoban']))
				$this->config['autoban'] = array_merge($this->config['autoban'],
					$config_ini['autoban']);
		} else {
			$this->config['autoban']['enabled'] = false;
		}
		foreach ($optconfig as $var => $val) {
			$this->config['autoban'][$var] = $val;
		}
	}
	/*--------------------------------------------------------------------------
	Start to count how many time we watch an IP address $ip by adding it to the
	'watch' NFT set and use the number of seconds of its timeout > 'watchtime'
	as a counter. If count is > 'maxcount' add $ip to 'jail' and 'parole' NFT 
	sets with timeouts 'jailtime' and jailtime'+'watchtime'. If $ip is watched
	during its parole immediately add it again to the 'jail' and 'parole' NFT sets 
	with timeouts 'repeatmult' longer than previously.

	@param  string  $ip  eg "23.94.144.50"
	@return boolean success
	*/
	public function book($ip) {
		// if not already in jail but on parole or watch count > maxcount
		// determine sentence and increment jail and parole counters
		if(ip2long($ip) === false) {
			trigger_error(sprintf('Got invalid IP address (%s)',$ip),E_USER_WARNING);
			return false;
		}
		$log = null;
		$watch_nft  = $this->timeoutsec('watch',$ip);
		$jail_nft   = $this->timeoutsec('jail',$ip);
		$parole_nft = $this->timeoutsec('parole',$ip);
		$watch_new  = $this->incrementwatch($watch_nft);
		$jail_new   = $this->jailsec($watch_new,$jail_nft,$parole_nft);
		$parole_new = $this->parolesec($jail_new);
		if ($jail_nft === false) {
			if($jail_new !== false) {
				$log = 'jail';
				$watch_new = false;
				if ($parole_nft !== false) $this->del('parole',$ip);
			} else {
				$log = 'watch';
				if ($watch_nft !== false) $this->del('watch',$ip);
			}
		}
		if ($this->add('watch',$ip,$watch_new) === false) return false;
		if ($this->add('jail',$ip,$jail_new) === false) return false;
		if ($this->add('parole',$ip,$parole_new) === false) return false;
		switch ($log) {
			case 'watch':
				$this->log(sprintf('Watching %15s %-8d',$ip,
					$this->countwatch($watch_new)));
				break;
			case 'jail':
				$this->log(sprintf('Jailing  %15s %8s',$ip,
					$this->timestr($jail_new)),null,E_USER_WARNING);
				break;
		}
		return true;
	}
	/*--------------------------------------------------------------------------
	Increment both watch count and watchtime, illustrated below, watchtime=20m.
		$time $count $time
		false      1 20m1s
		20m1s      2 20m2s
		20m2s      3 20m3s
		20m3s      4 20m4s
		20m4s      5 20m5s

	@param  mixed   $time integer time in seconds or boolean false
	@return integer time + 1
	*/
	private function incrementwatch($time) {
		if($time === false) {
			return $this->configsec('watchtime') + 1;
		} else {
			return $time + 1;
		}
	}
	/*--------------------------------------------------------------------------
	@param  integer $time integer time in seconds
	@return integer count
	*/
	private function countwatch($time) {
		return $time - $this->configsec('watchtime');
	}
	/*--------------------------------------------------------------------------
	Compute sentencing time which is last jailtime times repeatmult if in parole.
	Sentencing is jailtime if first time offender watch count >= maxcount.
	Return false if already in jail or watch count < maxcount.

	@param  mixed $watchtime integer time in seconds or boolean false
	@param  mixed $jailtime integer time in seconds or boolean false
	@param  mixed $paroletime integer time in seconds or boolean false
	@return mixed integer sentence time in seconds or boolean false
	*/
	private function jailsec($watchtime,$jailtime,$paroletime) {
		if ($jailtime !== false) return false;
		if ($paroletime !== false) {
			$jailt = max($this->configsec('jailtime'),
				$paroletime - $this->configsec('watchtime'));
			return $jailt * $this->config['autoban']['repeatmult'];
		} elseif (($watchtime !== false) &&
			($watchtime - $this->configsec('watchtime') >=
			$this->config['autoban']['maxcount'])) {
			return $sentence = $this->configsec('jailtime');
		}
		return false;
	}
	/*--------------------------------------------------------------------------
	Compute probation time = sentence time + watchtime. Also make sure both
	probation and sentence times are sane.

	@param  mixed &$sentence integer time in seconds or boolean false
	@return mixed integer probation time in seconds or boolean false
	*/
	private function parolesec(&$sentence) {
		if ($sentence === false) return false;
		$watchtime = $this->configsec('watchtime');
		if ($watchtime > 0.5*self::TIME_MAXSEC) $watchtime = 0.5*self::TIME_MAXSEC;
		$parole = $sentence + $watchtime;
		if ($parole > self::TIME_MAXSEC) {
			$parole = self::TIME_MAXSEC;
			$sentence = $parole - $watchtime;
		}
		$sentence = round($sentence);
		return round($parole);
	}
	/*--------------------------------------------------------------------------
	@param  string $set eg "jail"
	@param  string $ip  eg "23.94.144.50"
	@return mixed  time integer seconds or boolean false
	*/
	public function timeoutsec($set,$ip) {
		return $this->str2sec(get_timeout($set,$ip));
	}
	/*--------------------------------------------------------------------------
	@param  string $set eg "jail"
	@return mixed  time integer seconds or boolean false
	*/
	public function configtime($set) {
		switch ($set) {
		case 'watch':
			$sec = $this->configsec('watchtime');
			break;
		case 'jail':
			$sec = $this->configsec('jailtime');
			break;
		case 'parole':
			$sec = $this->configsec('jailtime') + $this->configsec('watchtime');
			break;
		default:
			$sec = null;
		}
		return $this->sec2str($sec);
	}
	/*--------------------------------------------------------------------------
	Convert config times in sting format to seconds eg "20m" to 1200
	@param  string  $param eg, "watchtime"
	@return integer $seconds
	*/
	public function configsec($param) {
		$time = $this->config['autoban'][$param];
		if(!is_numeric($time)) $time = $this->str2sec($time);
		return $time;
	}
	/*--------------------------------------------------------------------------
	@param  string  $message eg "Jailing 23.94.144.50"
	@param  mixed   $error eg 404
	@param  integer $level eg E_USER_WARNING
	@return void
	*/
	public function log($message, $error = [], $level = E_USER_NOTICE) {
		if (isset($error[0])) {
			$message = $message.' error: '.$error[0];
		} else {
			$nr_watch = count($this->list('watch'));
			$nr_jail = count($this->list('jail'));
			$message = sprintf('%s (watch %-3d jail %-3d)',
				$message,$nr_watch,$nr_jail);
		}
		trigger_error($message, $level);
	}
}
?>
