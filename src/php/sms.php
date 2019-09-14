#!/usr/bin/env php
<?php
/**
* sms_send.php
*
* This PHP script takes command line arguments and generates a (curl) http request
* to the ITSP web API which will send SMS.
*
* Usage
* Call via AGI in extensions.conf:
* same = n,AGI(sms.php,${EXTEN},${MESSAGE(from)},${MESSAGE(body)})
*
* Outline
* Define error handler and load variable values.
* Parse arguments.
* Build http POST query.
* Setup curl for POST query.
* Setup curl's authentication method
* Send POST query.
* Check responce and set exit code accordingly.
*
*/
require_once 'sms-common.php';

/**
* Parse arguments.
* We require 3 arguments; to, from, body.
*/
$sms_to   = @$argv[1] or trigger_error("no (to) argument", E_USER_WARNING);
$sms_from = @$argv[2] or trigger_error("no (from) argument", E_USER_WARNING);
$sms_body = @$argv[3] or trigger_error("no (body) argument", E_USER_WARNING);
is_numeric($sms_to) or trigger_error("(to=$sms_to) is not numeric", E_USER_WARNING);
is_numeric($sms_from) or trigger_error("(from=$sms_from) is not numeric", E_USER_WARNING);

/**
 Build http POST query,
 from arguments, sort and URL-encode.
*/
$sms_data = array_combine(
	array($sms_key_to, $sms_key_from, $sms_key_body),
	array($sms_to, $sms_from, $sms_body));
ksort($sms_data);
$sms_post_data = http_build_query($sms_data);
$sms_url = $sms_host.$sms_path;

/**
* Setup curl for POST query.
*/
$h_curl = curl_init($sms_url);
curl_setopt($h_curl, CURLOPT_RETURNTRANSFER, true);
curl_setopt($h_curl, CURLOPT_POST, true);
curl_setopt($h_curl, CURLOPT_POSTFIELDS, $sms_post_data);
curl_setopt($h_curl, CURLOPT_SSL_VERIFYPEER, false);
curl_setopt($h_curl, CURLINFO_HEADER_OUT, true); // debug only

/**
* Setup curl's authentication method.
* Currently we suport:
*
* 'basic'
*   basic access authentication, see, wikipedia.org/wiki/Basic_access_authentication,
*   with headers like: Authorization: Basic <credentials>
*
* 'zadarma'
*   Zadarma's uniqe authentication method, see, zadarma.com/en/support/api,
*   with headers like: Authorization: <user_key>:<signature>
*/
switch ($sms_auth_method) {
	case 'basic':
		curl_setopt($h_curl, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
		curl_setopt($h_curl, CURLOPT_USERPWD, "$sms_auth_user:$sms_auth_passwd");
		break;
	case 'zadarma':
		$sms_signature = base64_encode(hash_hmac('sha1', $sms_path .
			$sms_post_data . md5($sms_post_data), $sms_auth_passwd));
		curl_setopt($h_curl, CURLOPT_HTTPHEADER, array(
			'Authorization: ' . $sms_auth_user . ':' . $sms_signature,
			'Content-Type: application/json'));
		break;
	default:
		trigger_error("Unknown method (sms_auth_method=$sms_auth_method)", E_USER_WARNING);
		break;
}

/**
* Send POST query,
* read responce and close.
*/
$resp_json = curl_exec($h_curl) or trigger_error("Curl error: ".curl_error($h_curl));
$info = curl_getinfo($h_curl);
curl_close($h_curl);

/**
* Check responce and set exit code accordingly.
* the reponce check is controlled by the variable sms_resp_test. It can be:
* = null\""; No check, we will always exit with status true
* = "key=value" If value of key in responce is equal to value exit true otherwize false
* = "/pattern/"; If pattern matches responce exit with status true otherwize false
{
    "status":"success",
    "messages":1,
    "cost":0.24,
    "currency":"USD"
}
*/
$resp_keyval = json_decode($resp_json,true);
if (!empty($sms_resp_test)) {
	if (strpos($sms_resp_test, '=') !== false) {
		// "key=value"
		list($test_key, $test_value) = explode('=',$sms_resp_test);
		if (@$resp_keyval[$test_key] !== $test_value) 
			trigger_error("Called ($sms_url) but return key ($test_key = $resp_keyval[$test_key] != $test_value)", E_USER_WARNING);
	} else {
		// "/pattern/"
		if (!preg_match($sms_resp_test,$resp_json))
			trigger_error("Called ($sms_url) but return not matched ($resp_json != $sms_resp_test)", E_USER_WARNING);
	}
}

/**
* Close log and exit with appropriate status.
* The global variable $sms_exit_code will here be 0 if no errors was triggered.
*/
closelog();
if ($sms_exit_code === 0) {
	trigger_error("Outbound SMS to ($sms_to) sent via ($sms_url) successfully", E_USER_NOTICE);
}

exit($sms_exit_code);
?>

