<?php
/*------------------------------------------------------------------------------
 websmsd.php

 This PHP script listens to http requests, representing incoming SMS,
 from your ITSP and generate call files which will be picked up by asterisk.

 Usage
 Run with the PHP built-in web server:
 php -S 0.0.0.0:80 /path/websmsd.php

 Outline
 Define error handler and load variable values.
 Respond to echo requests.
 Read the post header data
 Generate call file name.
 Create new call file in the staging directory.
 Move the call file to the outgoing directory, so that Asterisk pick it up.
 Respond with a status message.
*/

openlog("websmsd", LOG_PID, LOG_LOCAL0);

require_once 'error.inc';
require_once 'websms.class.inc';
require_once 'astqueue.class.inc';

$sms = new Websms('/etc/asterisk/websms.conf');
$queue = new Astqueue('/etc/asterisk/websms.conf');

$message = $sms->rx_query();
$status = $queue->text($message);
$sms->ack_query($status, $message);
?>
