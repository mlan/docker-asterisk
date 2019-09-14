<?php
/**
* sms-common.php
*
* include this file in other files:
* require_once 'sms-common.php';
*
* Opens syslog.
* Define default variable values.
* Define error handler.
* Read ini file and load user defined variable values.
*/

/**
* Open syslog.
* Include the process ID and also send the log to standard error,
* and use a user defined logging mechanism.
*/
openlog("asterisk-sms", LOG_PID, LOG_LOCAL0);

/**
* Define default variable values,
* and extract them into current context.
*/
$sms_defaults = array(
	'sms_inifile'          => "/etc/asterisk/sms.conf",
	'sms_host'             => "https://api.twilio.com",
	'sms_path'             => "/2010-04-01/Accounts/xxx/Messages.json",
	'sms_key_to'           => "To",
	'sms_key_from'         => "From",
	'sms_key_body'         => "Body",
	'sms_auth_user'        => "usename",
	'sms_auth_passwd'      => "pwdorsecretkey",
	'sms_auth_method'      => "basic",
	'sms_resp_test'        => null,
	'smsd_outgoingdir'     => "/var/spool/asterisk/outgoing",
	'smsd_stagingdir'      => "/var/spool/asterisk/staging",
	'smsd_filemode'        => "",
	'smsd_fileowner'       => "",
	'smsd_waittime'        => 45,
	'smsd_maxretries'      => 0,
	'smsd_retrytime'       => 300,
	'smsd_archive'         => "no",
	'smsd_exten_context'   => "default",
	'smsd_message_context' => "default",
	'smsd_priority'        => 1,
	'smsd_key_to'          => "To",
	'smsd_key_from'        => "From",
	'smsd_key_body'        => "Body",
	'smsd_key_echo'        => null,
	'smsd_key_account'     => null,
	'smsd_resp_success'    => ""
);
extract($sms_defaults);


/**
* Define error handler,
* and activating it.
* Also set the golbal variable $sms_exit_code to its defalut value (0).
* An error is triggerd by calling:
* trigger_error(message, type)
* type is one of E_USER_ERROR, E_USER_WARNING, E_USER_NOTICE (the default).
*/
function smsErrorHandler($errno, $errstr, $errfile, $errline) {
	global $sms_exit_code;
	if (!(error_reporting() & $errno)) return false;
	$err_core_text = "[$errno]: ".basename($errfile).":$errline: $errstr";
	switch ($errno) {
	case E_USER_ERROR:
		$err_log_code = LOG_ERR;
		$err_log_text = "ERROR$err_core_text, aborting.";
		$sms_exit_code = 1;
		break;
	case E_USER_WARNING:
	case E_WARNING:
	case E_NOTICE:
		$err_log_code = LOG_WARNING;
		$err_log_text = "WARNING$err_core_text.";
		$sms_exit_code = 1;
		break;
	case E_USER_NOTICE:
		$err_log_code = LOG_NOTICE;
		$err_log_text = "NOTICE$err_core_text.";
		break;
	default:
		$err_log_code = LOG_DEBUG;
		$err_log_text = "UNKNOWN$err_core_text.";
		break;
	}
	syslog($err_log_code,$err_log_text);
	if(getenv('ASTERISK_SMSD_DEBUG')) echo $err_log_text.'<br>';
	if ($err_log_code === LOG_ERR) exit($sms_exit_code);
	return true; /* Don't execute PHP internal error handler */
}
$sms_exit_code = 0;
set_error_handler("smsErrorHandler");

/**
* Read ini file and load user defined variable values.
*/
$sms_ini = parse_ini_file($sms_inifile) or
	trigger_error("unable to open ini file ($sms_inifile)",E_USER_WARNING);
extract($sms_ini);

/**
* debug code
$arr = get_defined_vars();
print_r($arr);
*/

?>

