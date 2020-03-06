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
	private const SHOW_MARK = 'X';
	private const SHOW_ID = 'addr';
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
		$nft_file = (getenv('DOCKER_NFT_FILE') !== false) ?
			getenv('DOCKER_NFT_FILE') : self::NFT_FILE;
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
		if ($this->add_addrs($set,$addr,$timeout) === false) return false;
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
		if ($this->del_addrs($set,$addr) === false) return false;
		if ($this->save() === false) return false;
		return true;
	}
	/*--------------------------------------------------------------------------
	Get array of all addr in set $set.
	@param  string $set eg "jail"
	@return array  [string addr => string time, ...] or [] is set is empty
	*/
	public function list($set) {
		$return = $this->list_addr($set);
		if ($return === false) return [];
		return $this->array_elem($return);
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
	@param  mixed  $addrs string eg "23.94.144.50" or array ["23.94.144.50",...] or null or false
	@param  mixed  $timeout int seconds eg 1200 or boolean false
	@return mixed  exec return string or boolean false if unable to add elements
	*/
	public function add_addrs($set,$addrs,$timeout = null) {
		if (empty($addrs) || $timeout === false) return true;
		$suffix = (empty($timeout)) ? '' : ' timeout '.$this->timestr($timeout);
		if(is_array($addrs)) {
			$addrs = array_map(function($a) use ($suffix) { return $a.$suffix; }, $addrs);
			$val = implode(', ', $addrs);
		} else {
			$val = $addrs.$suffix;
		}
		$args = ['sub'=>'add element', 'set'=>$set, 'val'=>'{'.$val.'}'];
		return $this->exec($args);
	}
	/*--------------------------------------------------------------------------
	Delete element $addr from NFT set $set
	@param  string $set eg "jail"
	@param  mixed  $addrs string eg "23.94.144.50" or array ["23.94.144.50","all",...] or null
	@return mixed  exec return string or boolean false if unable to del elements
	*/
	public function del_addrs($set, $addrs = null) {
		if (empty($addrs)) return true;
		$given_addrs = (is_array($addrs)) ? $addrs : [$addrs];
		if (array_search('all', $given_addrs) !== false) {
			$args = ['sub'=>'flush set','set'=>$set];
		} else {
			$valid_addrs = array_keys($this->list($set));
			$the_addrs = array_intersect($given_addrs, $valid_addrs);
			if (empty($the_addrs)) return true;
			$val = implode(', ', $the_addrs);
			$args = ['sub'=>'delete element','set'=>$set,'val'=>'{'.$val.'}'];
		}
		return $this->exec($args);
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
		return $this->exec($args, false);
	}
	/*--------------------------------------------------------------------------
	List elements in NFT set $set
	@param  string $set eg "jail"
	@return mixed  exec return string or boolean false if unable to del element
	*/
	public function list_addr($set) {
		$args = ['sub'=>'list set','set'=>$set];
		return $this->exec($args);
	}
	/*--------------------------------------------------------------------------
	NFT returns elements = { 23.94.144.50 timeout 40m, ...}
	We stuff this into $this->timeout[$set] = [string addr => string time, ...].

	@param  array  $return strings return from calling NFT
	@return array  [string addr => string time, ...] or [] is set is empty
	*/
	public function array_elem($return) {
		if ($return === false) return [];
		preg_match('/flags timeout/', implode($return), $hastimeout);
		preg_match('/elements = {([^}]+)}/', implode($return), $matches);
		if (empty($matches[1])) return [];
		$elements = preg_split('/,/', $matches[1]);
		$timeout = [];
		foreach ($elements as $element) {
			if (empty($hastimeout)) {
				$timeout += [trim($element) => Self::SHOW_MARK];
			} else {
				$addrntime = explode(' timeout ',$element);
				$timeout += [trim($addrntime[0]) => trim($addrntime[1])];
			}
		}
		return $timeout;
	}
	/*--------------------------------------------------------------------------
	Save NFT state to file. Rename the old state file by appending "~" to the
	filename before saving the NFT state.
	@return mixed  state string or boolean false if unsuccessful
	*/
	public function save() {
		$args = ['sub'=>'list ruleset', 'family'=>null, 'table'=>null];
		$return = $this->exec($args);
		if ($return === false) return false;
		@rename($this->nft_statefile, $this->nft_statefile.'~');
		if (file_put_contents($this->nft_statefile,implode(PHP_EOL, $return))
			=== false) {
			$this->log('('.$this->nft_statefile.')', [ 'Unable to write' ],
				E_USER_WARNING);
			return false;
		}
		return $return;
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
	print table with headers: addr watch jail parole blacklist whitelist
	@return void
	*/
	public function show() {
		$db = [];
		$id = self::SHOW_ID;
		$headers = array_merge([$id],self::NFT_SETS);
		$sets_num = count(self::NFT_SETS);
		$form = "%15s".str_repeat("%13s", $sets_num)."\n";
		foreach (self::NFT_SETS as $set) {
			$elems = $this->list($set);
			if (!empty($elems)) foreach ($elems as $addr => $time) {
				$db_index = array_search($addr, array_column($db, $id));
				if ($db_index === false) {
					$db_index = -1 + array_push($db, array_merge([$id => $addr],
						array_combine(self::NFT_SETS,
						array_fill(0,$sets_num,null))));
				}
				$db[$db_index][$set] = $time;
			}
		}
		usort($db, function($a,$b) use ($id) {
			return ip2long($a[$id]) <=> ip2long($b[$id]);
		});
		$db_sums = array_map(function($head) use ($db) {
			return count(array_filter(array_column($db, $head)));
		}, $headers);
		vprintf($form,$headers);
		if (!empty($db)) foreach ($db as $elem) {
				vprintf($form,array_values($elem));
		}
		vprintf($form,$db_sums);

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
