<?php
/*------------------------------------------------------------------------------
 The Nft class provides functions edit and save the state of nftables.
*/
class Nft {
	private const NFT_SYNTAX = [
		'cmd'    => 'nft',
		'opt'    => '--stateless',
		'sub'    => null,
		'family' => 'inet',
		'table'  => 'autoban',
		'chain'  => null,
		'set'    => null,
		'val'    => null,
		'stderr' => '2>&1',
	];
	public const NFT_SETS = ['watch','jail','parole','blacklist','whitelist'];
	private const TIME_UNITS = ['d' => 86400, 'h' => 3600, 'm' => 60, 's' => 1];
	private const NFT_DIR = '/var/lib/nftables';
	private const NFT_FILE = 'autoban.nft';
	private $nft_statefile;
	public $debug = false;
	/*--------------------------------------------------------------------------
	Constructor
	@return void
	*/
	public function __construct() {
		$nft_dir = (getenv('DOCKER_NFT_DIR') !== false) ?
			getenv('DOCKER_NFT_DIR') : self::NFT_DIR;
		$nft_file = (getenv('DOCKER_NTF_FILE') !== false) ?
			getenv('DOCKER_NTF_FILE') : self::NFT_FILE;
		$this->nft_statefile = $nft_dir.'/'.$nft_file;
	}
	/*--------------------------------------------------------------------------
	Add element $addr to NFT set $set with timeout $timeout seconds
	@param  string $set eg "jail"
	@param  string $addr eg "23.94.144.50"
	@param  mixed  $timeout int seconds eg 1200 or boolean false
	@return boolean false if unable to add element else true
	*/
	public function add($set, $addr, $timeout = null, $check = false) {
		if ($check && !$this->is_addr($addr)) return false;
		if (!empty($timeout)) $timeout = $this->timestr($timeout);
		if ($this->add_addr($set,$addr,$timeout) === false) return false;
		if ($this->save() === false) return false;
		return true;
	}
	/*--------------------------------------------------------------------------
	Delete element $addr from NFT set $set
	@param  string $set eg "jail"
	@param  string $addr eg "23.94.144.50"
	@return mixed  boolean false if unable to del element else true
	*/
	public function del($set, $addr = null, $check = false) {
		if ($check) if (!$this->is_addr($addr)) return false;
		if ($this->del_addr($set,$addr) === false) return false;
		if ($this->save() === false) return false;
		return true;
	}
	/*--------------------------------------------------------------------------
	Get array of all addr in set $set.
	@param  string $set eg "jail"
	@return array  [string addr => string time, ...] or [] is set is empty
	*/
	public function list($set) {
		$ret = $this->list_addr($set);
		if ($ret === false) return [];
		return $this->array_elem($ret);
	}
	/*--------------------------------------------------------------------------
	Get timeout for $addr from NFT set $set
	@param  string $set eg "jail"
	@param  string $addr eg "23.94.144.50"
	@return mixed  exec return string or boolean false if unable to get timeout
	*/
	public function get_timeout($set,$addr) {
		$timeout = $this->array_elem($this->get_addr($set,$addr));
		if (!empty($timeout)) return array_values($timeout)[0];
		return false;
	}
	/*--------------------------------------------------------------------------
	Add element $addr to NFT set $set with timeout $timeout seconds
	@param  string $set eg "jail"
	@param  string $addr eg "23.94.144.50"
	@param  mixed  $timeout int seconds eg 1200 or boolean false
	@return mixed  exec return string or boolean false if unable to add element
	*/
	public function add_addr($set,$addr,$timeout = null) {
		if ($timeout === false) return true;
		$args = ['sub'=>'add element','set'=>$set];
		if (empty($timeout)) {
			$args['val'] = '{'.$addr.'}';
		} else {
			$args['val'] = '{'.$addr.' timeout '.$timeout.'}';
		}
		$ret = $this->exec($args);
		return $ret;
	}
	/*--------------------------------------------------------------------------
	Delete element $addr from NFT set $set
	@param  string $set eg "jail"
	@param  string $addr eg "23.94.144.50"
	@return mixed  exec return string or boolean false if unable to del element
	*/
	public function del_addr($set, $addr = null) {
		if (empty($addr)) {
			$args = ['sub'=>'flush set','set'=>$set];
		} else {
			$args = ['sub'=>'delete element','set'=>$set,'val'=>'{'.$addr.'}'];
		}
		$ret = $this->get_addr($set,$addr);
		if ($ret !== false) $ret = $this->exec($args);
		return $ret;
	}
	/*--------------------------------------------------------------------------
	Get element $addr from NFT set $set
	@param  string $set eg "jail"
	@param  string $addr eg "23.94.144.50"
	@return mixed  exec return string or boolean false if unable to del element
	*/
	public function get_addr($set,$addr) {
		if (empty($addr)) return true;
		$args = ['sub'=>'get element','set'=>$set,'val'=>'{'.$addr.'}'];
		$ret = $this->exec($args, false);
		return $ret;
	}
	/*--------------------------------------------------------------------------
	List elements in NFT set $set
	@param  string $set eg "jail"
	@return mixed  exec return string or boolean false if unable to del element
	*/
	public function list_addr($set) {
		$args = ['sub'=>'list set','set'=>$set];
		$ret = $this->exec($args);
		return $ret;
	}
	/*--------------------------------------------------------------------------
	NFT returns elements = { 23.94.144.50 timeout 40m, ...}
	We stuff this into $this->timeout[$set] = [string addr => string time, ...].

	@param  array  $ret strings return from calling NFT
	@return array  [string addr => string time, ...] or [] is set is empty
	*/
	public function array_elem($ret) {
		if ($ret === false) return [];
		preg_match('/flags timeout/', implode($ret), $hastimeout);
		preg_match('/elements = {([^}]+)}/', implode($ret), $matches);
		if (empty($matches[1])) return [];
		$elements = preg_split('/,/', $matches[1]);
		$timeout = [];
		foreach ($elements as $element) {
			if (empty($hastimeout)) {
				$timeout += [trim($element) => 'X'];
			} else {
				$addrntime = explode(' timeout ',$element);
				$timeout += [trim($addrntime[0]) => trim($addrntime[1])];
			}
		}
		return $timeout;
	}
	/*--------------------------------------------------------------------------
	Save NFT state to file.
	@return mixed  state string or boolean false if unsuccessful
	*/
	public function save() {
		$args = ['sub'=>'list ruleset', 'family'=>null, 'table'=>null];
		$ret = $this->exec($args);
		if ($ret === false) return false;
		if (file_put_contents($this->nft_statefile,implode(PHP_EOL, $ret))
			=== false) {
			$this->log('('.$this->nft_statefile.')', [ 'Unable to write' ],
				E_USER_WARNING);
			return false;
		}
		return $ret;
	}
	/*--------------------------------------------------------------------------
	@param  array  $args NFT cli arguments eg ['sub'=>'list set','set'=>'jail']
	@return mixed  NFT return string or boolean false if error status
	*/
	private function exec($args, $logerror = true) {
		$exec_array = array_merge(self::NFT_SYNTAX,$args);
		$exec_string = implode(' ',$exec_array);
		$this->debug($exec_string);
		exec($exec_string,$return,$status);
		if ($status === 0) {
			return $return;
		} else {
			if ($logerror) {
				$this->log('('.$exec_array['sub'].')', $return, E_USER_WARNING);
				$this->debug($exec_string);
			}
			return false;
		}
	}
	/*--------------------------------------------------------------------------
	@param  mixed $time string eg, "1d9h40m1s" integer 1200 or null
	@return string  $time eg, "1d9h40m1s" or null or boolean false
	*/
	public function timestr($time) {
		if (empty($time)) return $time;
		if (is_numeric($time)) return $this->sec2str($time);
		if ($this->str2sec($time) === false) {
			return false;;
		} else {
			return $time;
		}
	}
	/*--------------------------------------------------------------------------
	@param  string $time eg, "1d9h40m1s"
	@return mixed  $seconds int seconds or boolean false
	*/
	public function str2sec($time) {
		if ($time === false || $time === null) return $time;
		preg_match_all('/(\d+)([dhms])/',$time,$matches);
		if (empty($matches[0])) return false;
		$unitvalue = array_combine($matches[2],$matches[1]);
		$seconds = 0;
		foreach ($unitvalue as $unit => $value) {
			$seconds += self::TIME_UNITS[$unit] * $value;
		}
		return $seconds;
	}
	/*--------------------------------------------------------------------------
	@param  integer $seconds
	@return string  $time eg, "1d9h40m1s"
	*/
	public function sec2str($seconds) {
		if ($seconds === false || $seconds === null) return $seconds;
		$time = "";
		foreach (self::TIME_UNITS as $unit => $scale) {
			$number = floor($seconds / $scale);
			if ($number > 0) {
				$time .= sprintf('%d%s',$number,$unit);
				$seconds = $seconds % $scale;
			}
		}
		return $time;
	}
	/*--------------------------------------------------------------------------
	@param  string  $addr eg "23.94.144.50"
	@return boolean
	*/
	public function is_addr($addr) {
		if (ip2long($addr) === false) {
			trigger_error(sprintf('Got invalid IP address (%s)',
				$addr),E_USER_WARNING);
			return false;
		} else return true;
	}
	/*--------------------------------------------------------------------------
	@param  mixed $time string eg, "1d9h40m1s"
	@return boolean
	*/
	public function is_timestr($timeout) {
		if ($this->timestr($timeout) === false) {
			trigger_error(sprintf('Got invalid timeout value (%s)',
				$timeout),E_USER_WARNING);
			return false;
		} else return true;
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
		}
		trigger_error($message, $level);
	}
	/*--------------------------------------------------------------------------
	print table with headers: SADDR WATCH JAIL PAROLE BLACKLIST WHITELIST
	@return void
	*/
	public function show() {
		$list = [];
		$format = "%15s%13s%13s%13s%13s%13s\n";
		foreach (self::NFT_SETS as $set) {
			$tmp = $this->list($set);
			if (empty($tmp)) {
				$list[$set] = [];
			} else {
				$list[$set] = $tmp;
			}
		}
		$addrs = array_keys(array_merge($list['watch'],$list['jail'],
			$list['parole'],$list['blacklist'],$list['whitelist']));
		usort($addrs, function($a,$b) { return ip2long($a) <=> ip2long($b); });
		printf ($format,'SADDR','WATCH','JAIL','PAROLE','BLACKLIST',
			'WHITELIST');
		if (!empty($addrs)) {
			foreach ($addrs as $addr) {
				$elements[$addr] = [@$list['watch'][$addr],
					@$list['jail'][$addr],@$list['parole'][$addr],
					@$list['blacklist'][$addr], @$list['whitelist'][$addr]];
				printf ($format, $addr, @$list['watch'][$addr],
					@$list['jail'][$addr], @$list['parole'][$addr],
					@$list['blacklist'][$addr], @$list['whitelist'][$addr]);
			}
		}
	}
	/*--------------------------------------------------------------------------
	@param  mixed $param
	@return void
	*/
	public function debug($data, $debug = false) {
		if($debug || $this->debug) {
			var_dump($data);
		}
	}
}
?>
