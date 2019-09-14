<?php
/**
* smsd.php
*
* This PHP script listens to http requests, representing incoming SMS,
* from your ITSP and generate call files which will be picked up by asterisk.
*
* Usage
* Run with the PHP built-in web server:
* php -S 0.0.0.0:80 /path/smsd.php
* 
* Outline
* Define error handler and load variable values.
* Respond to echo requests.
* Read the post header data
* Generate call file name.
* Create new call file in the staging directory.
* Move the call file to the outgoing directory, so that Asterisk pick it up.
* Respond with a status message.
*
* Syntax of the Asterisk call file:
* Channel: The channel to use for the new call
* CallerID: The caller id to use
* Maxretries: Number of retries before failing
* RetryTime: How many seconds to wait before retry
* Context: The context in the dialplan
* Extension: The extension in the specified context
* Priority: he priority of the specified extension
* Setvar: MESSAGE(body)= The SMS message
* Setvar: MESSAGE(to)= The SMS destination
* Setvar: MESSAGE(from)= The SMS originator
*/
require_once 'sms-common.php';

/**
* Respond to echo requests.
* The API of some ITSP, eg Zadarma, test the web server by sending an echo
* request. Let's respond and exit if we detect a echo request.
*/
if (!empty($smsd_key_echo) && isset($_GET[$smsd_key_echo]))
	exit($_GET[$smsd_key_echo]);

/**
* Read the post header data.
* Also log warnings if any header data is missing.
*/
$smsd_data = array();
$post_data = file_get_contents("php://input") or
	trigger_error("unable to get header data", E_USER_WARNING);
parse_str($post_data, $smsd_data);

if (empty($smsd_data)) trigger_error("no header data", E_USER_WARNING);
if (empty($smsd_data[$smsd_key_to]))
	trigger_error("no ($smsd_key_to) key in header data", E_USER_WARNING);
if (empty($smsd_data[$smsd_key_from]))
	trigger_error("no ($smsd_key_from) key in header data", E_USER_WARNING);
if (empty($smsd_data[$smsd_key_body]))
	trigger_error("no ($smsd_key_body) key in header data", E_USER_WARNING);
if (!empty($smsd_key_account) && empty($smsd_data[$smsd_key_account]))
	trigger_error("no ($smsd_key_account) key in header data", E_USER_WARNING);

/**
* Generate call file name.
* Format: <EXTEN>.<seconds since the Unix Epoch>.<3 digit random>.call
*/
if ($sms_exit_code === 0) {
$smsd_filename = $smsd_data[$smsd_key_to].".".time().".".rand(100,999).".call";
$smsd_stagingfile = $smsd_stagingdir."/".$smsd_filename;
$smsd_outgoingfile = $smsd_outgoingdir."/".$smsd_filename;
}

/**
* Create new call file in the staging directory.
* Using staging directory to avoid Asterisk reading an unfinished file.
*/
if ($sms_exit_code === 0) {
$h_file = fopen($smsd_stagingfile, "w") or
	trigger_error("unable to open call file ($smsd_stagingfile)", E_USER_WARNING);
fwrite($h_file, "#".PHP_EOL."# POST HEADER: ".$post_data.PHP_EOL."#".PHP_EOL.PHP_EOL);
fwrite($h_file, "Channel: Local/".$smsd_data[$smsd_key_to]."@".$smsd_exten_context.PHP_EOL);
fwrite($h_file, "CallerID: \"\" <".$smsd_data[$smsd_key_from].">".PHP_EOL);
fwrite($h_file, "WaitTime: ".$smsd_waittime.PHP_EOL);
fwrite($h_file, "MaxRetries: ".$smsd_maxretries.PHP_EOL);
fwrite($h_file, "RetryTime: ".$smsd_retrytime.PHP_EOL);
fwrite($h_file, "Archive: ".$smsd_archive.PHP_EOL);
if (!empty($smsd_key_account))
	fwrite($h_file, "Account: ".$smsd_data[$smsd_key_account].PHP_EOL);
fwrite($h_file, "Context: ".$smsd_message_context.PHP_EOL);
fwrite($h_file, "Extension: ".$smsd_data[$smsd_key_to].PHP_EOL);
fwrite($h_file, "Priority: ".$smsd_priority.PHP_EOL);
fwrite($h_file, "Setvar: MESSAGE(to)=".$smsd_data[$smsd_key_to].PHP_EOL);
fwrite($h_file, "Setvar: MESSAGE(from)=".$smsd_data[$smsd_key_from].PHP_EOL);
fwrite($h_file, "Setvar: MESSAGE(body)=".$smsd_data[$smsd_key_body].PHP_EOL);
fclose($h_file) or
	trigger_error("unable to close call file ($smsd_stagingfile)", E_USER_WARNING);
}

/**
* Move the call file to the outgoing directory, so that Asterisk pick it up.
*
*
* if (!empty($smsd_filemode)) chmod($smsd_stagingfile, octdec($smsd_filemode));
* if (!empty($smsd_fileowner)) chown($smsd_stagingfile, $smsd_fileowner);
*/
if ($sms_exit_code === 0)
	rename($smsd_stagingfile, $smsd_outgoingfile) or
		trigger_error("unable to move file ($smsd_stagingfile) to file ($smsd_outgoingfile)", E_USER_WARNING);

/**
* Respond with a status message.
* The global variable $sms_exit_code will here be 0 if no errors was triggered.
*/
if ($sms_exit_code === 0) {
	trigger_error("Inbount SMS received, created call file ($smsd_filename)", E_USER_NOTICE);
	echo $smsd_resp_success;
}
?>

