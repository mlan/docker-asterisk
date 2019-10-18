#!/usr/bin/env php
<?php
/**
* websms.php
*
* This PHP script takes command line arguments and generates a (curl) http request
* to the ITSP web API which will send SMS.
*
* Usage
* Call via AGI in extensions.conf:
* same = n,AGI(websms.php,${EXTEN},${MESSAGE(from)},${MESSAGE(body)})
*
*/

openlog("websms", LOG_PID | LOG_PERROR, LOG_LOCAL0);

require_once 'error.inc';
require_once 'websms.class.inc';

$sms = new Websms('/etc/asterisk/websms.conf');

// Send POST querry and check responce and set exit code accordingly.
if ($sms->query(@$argv)) {
	$exit_code = 0;
} else {
	$exit_code = 1;
}

closelog();

exit($exit_code);
?>
