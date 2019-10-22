<?php
/**
* The AmiUpdateConfig class provides functions updating asterisk ACL
* configuration so that banned/unbanned ip can be denied/permitted access.
* This class extends the phpami class Ami.
* $ami->amiUpdate('Append',['Value-000000' => $ip]);
* $ami->amiUpdate('Delete',['Match-000000' => $ip]);
* $ami->amiUpdate('Delete');

*/
require_once 'ami-class.php';

class AmiUpdateConfig extends \PHPAMI\Ami {
	const DEFAULT_CONF_FILE = '/etc/asterisk/autoban.conf';
	const DEFAULT_CONF_VALS = [
		'Action'                   => 'UpdateConfig',
		'ActionID'                 => 0,
		'SrcFilename'              => 'pjsip.conf',
		'DstFilename'              => 'pjsip.conf',
		'Reload'                   => 'res_pjsip',
		'PreserveEffectiveContext' => false,
		'Action-000000'            => 'Append',
		'Cat-000000'               => 'autoban',
		'Var-000000'               => 'deny',
		'Value-000000'             => null,
		'Match-000000'             => null,
		'Line-000000'              => null
	];
	public function __construct($config = null, array $optconfig = []) {
		parent::__construct($config,$optconfig);
		if (is_string($config) !== true) {
			$config = self::DEFAULT_CONF_FILE;
		}
		$this->config['amiaction'] = self::DEFAULT_CONF_VALS;
		if (file_exists($config) === true) {
			$config_ini = parse_ini_file($config,true);
			$this->config['amiaction'] = array_merge($this->config['amiaction'],$config_ini['amiaction']);
		}
	}
	public function amiUpdate($action,$args = []) {
		$parameters = array_merge($this->config['amiaction'],
			[ 'Action-000000' => $action ], $args);
		$res = $this->sendRequest('UpdateConfig',$parameters);
		if ($res['Response'] !== 'Success') {
			$this->log("$action failed.",self::LOG_ERROR);
			var_dump($parameters);
			var_dump($res);
			return false;
			}
		$this->log("$action Success",self::LOG_WARN);
		return true;
	}
	public function amiGetIp($parameters) {
		if (is_array($parameters) && array_key_exists('RemoteAddress',$parameters)) {
			$address = explode('/',$parameters['RemoteAddress']);
			$ip = $address[2];
			return $ip;
		} else {
			return false;
		}
	}
	public function amiGetValue($parameters,$key) {
		if (is_array($parameters) && array_key_exists($key,$parameters)) {
			return $parameters[$key];
		} else {
			return false;
		}
	}
}
?>
