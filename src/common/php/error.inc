<?php
/**
* Define error handler,
* and activating it.
* An error is triggerd by calling:
* trigger_error(message, type)
* type is one of E_USER_ERROR, E_USER_WARNING, E_USER_NOTICE (the default).
*/
function errorHandler($errno, $errstr, $errfile, $errline) {
	if (!(error_reporting() & $errno)) return false;
	$err_core_text = "[$errno]: ".basename($errfile).":$errline: $errstr";
	switch ($errno) {
	case E_USER_ERROR:
		$err_log_code = LOG_ERR;
		$err_log_text = "ERROR$err_core_text.";
		break;
	case E_USER_WARNING:
	case E_WARNING:
	case E_NOTICE:
		$err_log_code = LOG_WARNING;
		$err_log_text = "WARNING$err_core_text.";
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
	return true; /* Don't execute PHP internal error handler */
}
set_error_handler("errorHandler");
?>
