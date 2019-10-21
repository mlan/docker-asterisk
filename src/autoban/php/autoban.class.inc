<?php
/*------------------------------------------------------------------------------
 The Autoban class provides functions for watching and jailing IP addresses
 using nftables.
------------------------------------------------------------------------------*/
class Autoban {
	private const DEFAULT_CONF_FILE = '/etc/asterisk/nft.conf';
	public const DEFAULT_CONF_VALS = [
		'enabled'     => true,
		'maxcount'    => 10,
		'watchtime'   => 1200,
		'jailtime'    => 1200,
		'relapsebase' => 6
	];
	private const DEFAULT_NFT_VALS = [
		'cmd'    => 'nft',
		'sub'    => null,
		'family' => 'inet',
		'table'  => 'autoban',
		'chain'  => null,
		'set'    => null,
		'pre'    => null,
		'val'    => null,
		'pst'    => null,
		'opt'    => null
	];
	private const TIMEUNIT = ['d' => 86400, 'h' => 3600, 'm' => 60, 's' => 1];
	private const MAXTIME = 99999999;
	public $config;
	private $timeout = [ 'watch' => [], 'jail' => [], 'parole' => [] ];
	public $debug = false;
	public function __construct($config = null, array $optconfig = []) {
		if (is_string($config) !== true) {
			$config = self::DEFAULT_CONF_FILE;
		}
		$this->config['autoban'] = self::DEFAULT_CONF_VALS;
		$this->config['nftables'] = self::DEFAULT_NFT_VALS;
		if (file_exists($config) === true) {
			$config_ini = parse_ini_file($config,true);
			$this->config['autoban'] = array_merge($this->config['autoban'],
				$config_ini['autoban']);
			$this->config['nftables'] = array_merge($this->config['nftables'],
				$config_ini['nftables']);
		} else {
			$this->config['autoban']['enabled'] = false;
		}
		foreach ($optconfig as $var => $val) {
			$this->config['autoban'][$var] = $val;
		}
	}
	/*--------------------------------------------------------------------------
	The number of offence from an IP address $ip is initially
	
	@param  string  $ip  eg "23.94.144.50"
	@return boolean success
	--------------------------------------------------------------------------*/
	public function book($ip) {
		// if not already in jail but on parole or watch count > maxcount
		// determine sentence and increment jail and parole counters
		if(ip2long($ip) === false) {
			trigger_error(sprintf('Got invalid IP address (%s)',$ip),E_USER_WARNING);
			return false;
		}
		$log = null;
		$watchtime = $this->gettimeout('watch',$ip);
		$jailtime = $this->gettimeout('jail',$ip);
		$paroletime = $this->gettimeout('parole',$ip);
		$sentence = $this->sentence($watchtime,$jailtime,$paroletime);
		$probation = $this->probation($sentence);
		if ($jailtime === false) {
			if($sentence !== false) {
				$log = 'jail';
				if ($paroletime !== false) $this->del('parole',$ip);
			} else {
				$log = 'watch';
				if ($watchtime !== false) $this->del('watch',$ip);
			}
		}
		$count = $this->increment('watchtime',$watchtime);
		if ($this->add('watch',$ip,$watchtime) === false) return false;
		if ($this->add('jail',$ip,$sentence) === false) return false;
		if ($this->add('parole',$ip,$probation) === false) return false;
		switch ($log) {
			case 'watch':
				$this->log(sprintf('Watching %15s = %-8d',$ip,$count));
				break;
			case 'jail':
				$this->log(sprintf('Jailing  %15s = %8s',$ip,
					$this->timeout($sentence)),null,E_USER_WARNING);
				break;
		}
		return true;
	}
	/*--------------------------------------------------------------------------
	Increment both watch count and watchtime, illustrated below, watchtime=20m.
		$time $count $time
		false      1   20m
		  20m      2 20m1s
		20m1s      3 20m2s
		20m2s      4 20m3s
		20m3s      5 20m4s

	@param  string  $param eg 'watchtime'
	@param  mixed   $time integer time in seconds or boolean false
	@return integer count
	--------------------------------------------------------------------------*/
	private function increment($param,&$time) {
		$time1 = $this->configtime($param);
		if($time === false) {
			$time = $time1;
			return 1;
		} else {
			$time++;
			return max(1, $time - $time1 + 2);
		}
	}
	/*--------------------------------------------------------------------------
	Compute sentencing time which is last jailtime times relapsebase if in parole.
	Sentencing is jailtime if first time offender watch count >= maxcount.
	Return false if already in jail or watch count < maxcount.
	
	@param  mixed $watchtime integer time in seconds or boolean false
	@param  mixed $jailtime integer time in seconds or boolean false
	@param  mixed $paroletime integer time in seconds or boolean false
	@return mixed integer sentence time in seconds or boolean false
	--------------------------------------------------------------------------*/
	private function sentence($watchtime,$jailtime,$paroletime) {
		if ($jailtime !== false) return false;
		if ($paroletime !== false) {
			$jailt = max($this->configtime('jailtime'),
				$paroletime - $this->configtime('watchtime'));
			return $jailt * $this->config['autoban']['relapsebase'];
		} elseif (($watchtime !== false) &&
			($watchtime - $this->configtime('watchtime') + 2 >=
			$this->config['autoban']['maxcount'])) {
			return $sentence = $this->configtime('jailtime');
		}
		return false;
	}
	/*--------------------------------------------------------------------------
	Compute probition time = sentence time + watchtime. Also make sure both 
	probition and sentence times are sane.
	
	@param  mixed &$sentence integer time in seconds or boolean false
	@return mixed integer probition time in seconds or boolean false
	--------------------------------------------------------------------------*/
	private function probation(&$sentence) {
		if ($sentence === false) return false;
		$watchtime = $this->configtime('watchtime');
		if ($watchtime > 0.5*self::MAXTIME) $watchtime = 0.5*self::MAXTIME;
		$parole = $sentence + $watchtime;
		if ($parole > self::MAXTIME) {
			$parole = self::MAXTIME;
			$sentence = $parole - $watchtime;
		}
		$sentence = round($sentence);
		return round($parole);
	}
	/*--------------------------------------------------------------------------
	Add element $ip to NFT set $set with timeout $timeout seconds
	@param  string $set eg "jail"
	@param  string $ip  eg "23.94.144.50"
	@param  mixed  $timeout int seconds eg 1200 or boolean false
	@return mixed  exec return string or boolean false if unable to add element
	--------------------------------------------------------------------------*/
	public function add($set,$ip,$timeout) {
		if ($timeout === false) return true;
		$args = ['sub'=>'add element','set'=>$set,'pre'=>'{ ','val'=>$ip,
			'pst'=>'timeout '.$timeout.'s }'];
		$ret = $this->exec($args);
		if ($ret === false) return false;
		return $ret;
	}
	/*--------------------------------------------------------------------------
	Delete element $ip from NFT set $set
	@param  string $set eg "jail"
	@param  string $ip  eg "23.94.144.50"
	@return mixed  exec return string or boolean false if unable to del element
	--------------------------------------------------------------------------*/
	public function del($set,$ip) {
		$args = ['sub'=>'delete element','set'=>$set,'pre'=>'{ ','val'=>$ip,
			'pst'=>' }'];
		$ret = $this->exec($args);
		if ($ret === false) return false;
		return $ret;
	}
	/*--------------------------------------------------------------------------
	NFT returns elements = { 23.94.144.50 timeout 40m expires 31m8s696ms, ...}
	We stuff this into the timeout arrays [sting addr => string time, ...].
	Now we can get the timeout for the IP address $ip

	@param  string $set eg "jail"
	@param  string $ip  eg "23.94.144.50"
	@return mixed  time integer seconds or boolean false
	--------------------------------------------------------------------------*/
	public function gettimeout($set,$ip) {
		$args = ['sub'=>'list set','set'=>$set];
		$ret = $this->exec($args);
		if ($ret === false) return false;
		preg_match('/elements = {([^}]+)}/', implode($ret), $matches);
		if (empty($matches[1])) return false;
		$elementsexpires = preg_split('/,/', $matches[1]);
		$elements = preg_replace('/ expires .*/','',$elementsexpires);
		$timeout = [];
		foreach ($elements as $element) {
			$addrntime = explode(' timeout ',$element);
			$timeout += [trim($addrntime[0]) => trim($addrntime[1])];
		}
		$this->timeout[$set]=$timeout;
		if (isset($timeout[$ip])) {
			return $this->seconds($timeout[$ip]);
		} else {
			return false;
		}
	}
	/*--------------------------------------------------------------------------
	@param  string  $set eg "jail"
	@return integer count elements
	--------------------------------------------------------------------------*/
	public function countelements($set) {
		$args = ['sub'=>'list set','set'=>$set];
		$ret = $this->exec($args);
		if ($ret === false) return false;
		preg_match('/elements = {([^}]+)}/', implode($ret), $matches);
		if (empty($matches[1])) return 0;
		$elementsexpires = preg_split('/,/', $matches[1]);
		return count($elementsexpires);
	}
	/*--------------------------------------------------------------------------
	@param  array  $args NFT cli arguments eg ['sub'=>'list set','set'=>'jail']
	@return mixed  NFT return string or boolean false if error status
	--------------------------------------------------------------------------*/
	private function exec($args) {
		$exec_array = array_merge($this->config['nftables'],$args);
		$exec_string = implode(' ',$exec_array);
		$this->debug($exec_string);
		exec($exec_string,$return,$status);
		if ($status === 0) {
			return $return;
		} else {
			$this->log('('.$exec_array['sub'].')', $return, E_USER_WARNING);
			$this->debug($exec_string);
			return false;
		}
	}
	/*--------------------------------------------------------------------------
	@param  string $time eg, "1d9h40m1s"
	@return mixed  $seconds int seconds or boolean false
	--------------------------------------------------------------------------*/
	public function seconds($time) {
		preg_match_all('/(\d+)([dhms])/',$time,$matches);
		if (empty($matches[0])) return false;
		$unitvalue = array_combine($matches[2],$matches[1]);
		$seconds = 0;
		foreach ($unitvalue as $unit => $value) {
			$seconds += self::TIMEUNIT[$unit] * $value;
		}
		return $seconds;
	}
	/*--------------------------------------------------------------------------
	@param  integer $seconds
	@return string  $time eg, "1d9h40m1s"
	--------------------------------------------------------------------------*/
	public function timeout($seconds) {
		$time = "";
		foreach (self::TIMEUNIT as $unit => $scale) {
			$number = floor($seconds / $scale);
			if ($number > 0) {
				$time .= sprintf('%d%s',$number,$unit);
				$seconds = $seconds % $scale;
			}
		}
		return $time;
	}
	/*--------------------------------------------------------------------------
	Convert config times in sting format to seconds eg "20m" to 1200
	@param  string  $param eg, "watchtime"
	@return integer $seconds
	--------------------------------------------------------------------------*/
	public function configtime($param) {
		$time = $this->config['autoban'][$param];
		if(!is_numeric($time)) $time = $this->seconds($time);
		return $time;
	}
	/*--------------------------------------------------------------------------
	@param  string  $message eg "Jailing 23.94.144.50"
	@param  mixed   $error eg 404
	@param  integer $level eg E_USER_WARNING
	@return void
	--------------------------------------------------------------------------*/
	public function log($message, $error = [], $level = E_USER_NOTICE) {
		if (isset($error[0])) {
			$message = $message.' error: '.$error[0];
		} else {
			$nr_watch = $this->countelements('watch');
			$nr_jail = $this->countelements('jail');
			$message = sprintf('%s (watch = %-3d jail = %-3d)',
				$message,$nr_watch,$nr_jail);
		}
		trigger_error($message, $level);
	}
	/*--------------------------------------------------------------------------
	@param  mixed $param
	@return void
	--------------------------------------------------------------------------*/
	public function debug($data, $debug = false) {
		if($debug || $this->debug) {
			var_dump($data);
		}
	}
	public function debugOLD($data, $debug = false) {
		if($debug || $this->debug) {
			if (is_string($data)) {
				$message = @sprintf("State    %15s w=%-2d j=%-2d p=%-2d\n",
					$data,$this->count['watch'][$data],$this->count['jail'][$data],
					$this->count['parole'][$data]);
				$this->log($message);
			} else {
				var_dump($data);
			}
		}
	}
}
?>
