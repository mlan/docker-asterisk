<?php
/**
* The Nft class provides functions for banning and unbanning source ip addessed
* using nftables.
* nft list set inet autoban watch
*/
namespace Autoban;
class Nft {
	private const DEFAULT_CONF_FILE = '/etc/asterisk/nft.conf';
	public const DEFAULT_CONF_VALS = [
		'enabled'     => true,
		'maxcount'    => 3,
		'watchtime'   => 600,
		'jailtime'    => 1800,
		'relapsebase' => 10
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
	private const MAXTIME = 99999999;
	public $config;
	private $count = [ 'watch' => [], 'jail' => [], 'parole' => [] ];
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
	public function book($ip) {
		// prune counters from ip that has timed out
		@printf("autoban before  prune %15s w=%-2d j=%-2d p=%-2d\n",$ip,$this->count['watch'][$ip],$this->count['jail'][$ip],$this->count['parole'][$ip]);
		$this->prune('watch');
		$this->prune('jail');
		$this->prune('parole');
		@printf("autoban after   prune %15s w=%-2d j=%-2d p=%-2d\n",$ip,$this->count['watch'][$ip],$this->count['jail'][$ip],$this->count['parole'][$ip]);
		// increment watch counter
		$this->inc('watch',$ip);
		// add ip to watch set
		$this->add('watch',$ip,$this->config['autoban']['watchtime']);
		// if not alreay in jail but on parole or watch count > maxcount
		// determine sentence and increment jail and parole counters
		if (isset($this->count['parole'][$ip])) {
			if (!isset($this->count['jail'][$ip])) {
				$this->inc('jail',$ip);
				$this->inc('parole',$ip);
				$this->del('parole',$ip);
			}
			$relapsecount = $this->count['parole'][$ip];
		} elseif ($this->count['watch'][$ip] >=
			$this->config['autoban']['maxcount']) {
			$this->inc('jail',$ip,false);
			$this->inc('parole',$ip,false);
			$relapsecount = $this->count['parole'][$ip];
		}
		// if relapsecount, determine sentence and add ip to jail and parole sets
		if (isset($relapsecount)) {
			$sentence = $this->config['autoban']['jailtime']
				* pow($this->config['autoban']['relapsebase'],
					max($relapsecount -1, 0));
			$parole = $this->chktime($sentence);
			$this->add('jail',$ip,$sentence);
			$this->add('parole',$ip,$parole);
			$this->log(sprintf('Jailing  %15s = %-3d',$ip,
				$this->count['parole'][$ip]),null,E_USER_WARNING);
		} else {
			$this->log(sprintf('Watching %15s = %-3d',$ip,
				$this->count['watch'][$ip]));
		}
		@printf("autoban after verdict %15s w=%-2d j=%-2d p=%-2d\n",$ip,$this->count['watch'][$ip],$this->count['jail'][$ip],$this->count['parole'][$ip]);
	}
	public function inc($set,$ip,$inc=true) {
		if (isset($this->count[$set][$ip]) && $inc) {
			$this->count[$set][$ip]++;
		} else {
			$this->count[$set][$ip] = 1;
		}
	}
	public function chktime(&$sentence) {
		$watchtime = $this->config['autoban']['watchtime'];
		if ($watchtime > 0.5*self::MAXTIME) $watchtime = 0.5*self::MAXTIME;
		$parole = $sentence + $watchtime;
		if ($parole > self::MAXTIME) {
			$parole = self::MAXTIME;
			$sentence = $parole - $watchtime;
		}
		$sentence = round($sentence);
		return round($parole);
	}
	public function prune($set) {
		$ipaddresses = $this->list($set);
		$this->debug($ipaddresses);
		if (empty($ipaddresses)) {
			$this->count[$set] = [];
		} else {
			$basecount = array_fill_keys($ipaddresses,1);
			$this->debug($basecount);
			foreach ($basecount as $ip => &$count) {
				if (array_key_exists($ip,$this->count[$set])) {
					$count = max($this->count[$set][$ip],1);
				}
			}
			unset($count);
			$this->debug($basecount);
			$this->count[$set] = $basecount;
		}
	}
	public function add($set,$ip,$timeout) {
		$args = ['sub'=>'add element','set'=>$set,'pre'=>'{ ','val'=>$ip,
			'pst'=>'timeout '.$timeout.'s }'];
		$ret = $this->exec($args);
		return $ret;
	}
	public function del($set,$ip) {
		$args = ['sub'=>'delete element','set'=>$set,'pre'=>'{ ','val'=>$ip,
			'pst'=>' }'];
		$ret = $this->exec($args);
		return $ret;
	}
	public function list($set) {
		$args = ['sub'=>'list set','set'=>$set];
		$ret = $this->exec($args);
		if ($ret === false) return false;
		preg_match('/elements = {([^}]+)}/', implode($ret), $matches);
		if (empty($matches[1])) return false;
		$elements = preg_split('/,/', $matches[1]);
		$ipaddresses = preg_replace('/^\s*([^ ]+).*/','$1',$elements);
		if (empty($ipaddresses)) return false;
		return $ipaddresses;
	}
	private function exec($args) {
		$exec_array = array_merge($this->config['nftables'],$args);
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
	public function log($message, $error = [], $level = E_USER_NOTICE) {
		//$message = 'nftables '.$message;
		if (isset($error[0])) {
			$message = $message.' error: '.$error[0];
		} else {
			$nr_watch = count($this->count['watch']);
			$nr_jail = count($this->count['jail']);
			$nr_parole = max(count($this->count['parole']) - $nr_jail,0);
			$message = sprintf('%s (watch = %-2d jail = %-2d)',
				$message,$nr_watch,$nr_jail);
		}
		trigger_error($message, $level);
	}
	public function debug($data, $debug = false) {
		if($debug || $this->debug)
			var_dump($data);
	}
}
?>

