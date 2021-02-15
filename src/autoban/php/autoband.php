#!/usr/bin/env php
<?php
/*------------------------------------------------------------------------------
 autoband.php
*/

/*------------------------------------------------------------------------------
 Initiate logging and load dependencies.
*/
openlog("autoband", LOG_PID, LOG_LOCAL0);
require_once 'error.inc';
require_once 'ami.class.inc';
require_once 'autoban.class.inc';

/*------------------------------------------------------------------------------
 Define AMI event handlers.
*/
function eventAbuse($event,$parameters,$server,$port) {
	global $ban;
	if (array_key_exists('RemoteAddress',$parameters)) {
		$address = explode('/',$parameters['RemoteAddress']);
		$ip = $address[2];
		if (!empty($ip)) {
			$ban->book($ip);
		}
	}
}

/*------------------------------------------------------------------------------
 Create class objects and set log level.
*/
$ban = new \Autoban('/etc/asterisk/autoban.conf');
$ami = new \PHPAMI\Ami('/etc/asterisk/autoban.conf');
$ami->setLogLevel(2);

/*------------------------------------------------------------------------------
 Register the AMI event handlers to their corresponding events.
*/
$ami->addEventHandler('FailedACL',               'eventAbuse');
$ami->addEventHandler('InvalidAccountID',        'eventAbuse');
$ami->addEventHandler('ChallengeResponseFailed', 'eventAbuse');
$ami->addEventHandler('InvalidPassword',         'eventAbuse');

/*------------------------------------------------------------------------------
 Start code execution.
 Wait 1s allowing Asterisk time to setup the Asterisk Management Interface (AMI).
 If autoban is activated try to connect to the AMI. If successful, start
 listening for events indefinitely. If connection fails, retry to connect.
 If autoban is deactivated stay in an infinite loop instead of exiting.
 Otherwise the system supervisor will relentlessly just try to restart us.
*/
$wait_init  = 2;
$wait_extra = 58;
$wait_off   = 3600;
if ($ban->config['autoban']['enabled']) {
	while(true) {
		sleep($wait_init);
		if ($ami->connect()) {
			trigger_error('Activated and connected to AMI',E_USER_NOTICE);
			$ami->waitResponse(); // listen for events until connection fails
			$ami->disconnect();
		} else {
			trigger_error('Unable to connect to AMI',E_USER_ERROR);
			sleep($wait_extra);
		}
	}
} else {
	trigger_error('Disabled! Activate autoban using conf file '.
	'(/etc/asterisk/autoban.conf)',E_USER_NOTICE);
	while(true) { sleep($wait_off); }
}


/*------------------------------------------------------------------------------
 We will never come here.
*/
?>
