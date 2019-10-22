#!/usr/bin/env php
<?php
/*------------------------------------------------------------------------------
 autoban.php
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
 Initiate logging and load dependencies.
------------------------------------------------------------------------------*/
openlog("autoban", LOG_PID, LOG_LOCAL0);
require_once 'error.inc';
require_once 'ami.class.inc';
require_once 'autoban.class.inc';

/*------------------------------------------------------------------------------
 Define AMI event handlers.
------------------------------------------------------------------------------*/
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
------------------------------------------------------------------------------*/
$ban = new \Autoban('/etc/asterisk/autoban.conf');
$ami = new \PHPAMI\Ami('/etc/asterisk/autoban.conf');
$ami->setLogLevel(2);

/*------------------------------------------------------------------------------
 Register the AMI event handlers to their corresponding events.
------------------------------------------------------------------------------*/
$ami->addEventHandler('FailedACL',               'eventAbuse');
$ami->addEventHandler('InvalidAccountID',        'eventAbuse');
$ami->addEventHandler('ChallengeResponseFailed', 'eventAbuse');
$ami->addEventHandler('InvalidPassword',         'eventAbuse');

/*------------------------------------------------------------------------------
 Start code execution.
 Wait 1s allowing Asterisk time to setup the Asterisk Management Interface (AMI).
 If autoban is activated try to connect to the AMI. If successful, start
 listening for events indefinitely. If connection fails, exit and let the
 system supervisor start us again, so we can retry to connect.
 If autoban is deactivated stay in an infinite loop instead of exiting.
 Otherwise the system supervisor will relentlessly just try to restart us.
------------------------------------------------------------------------------*/
sleep(1);
if ($ban->config['autoban']['enabled']) {
	if ($ami->connect(null,null,null,'on') === false) {
		$connected = false;
		trigger_error('Unable to connect to Asterisk Management Interface',E_USER_ERROR);
	} else {
		$connected = true;
		trigger_error('Activated and connected to Asterisk Management Interface',E_USER_NOTICE);
	}
	while($connected) { $ami->waitResponse(); }
} else {
	trigger_error('Disabled! Activate autoban using conf file (/etc/asterisk/autoban.conf)',E_USER_NOTICE);
	while(true) { sleep(60); }
}

/*------------------------------------------------------------------------------
 We normally will not come here.
------------------------------------------------------------------------------*/
$ami->disconnect();
?>
