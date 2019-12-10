#!/usr/bin/env php
<?php
/*------------------------------------------------------------------------------
 autoban.php
*/
$HELP_MESSAGE = <<<HELP_MESSAGE

  DESCRIPTION
    Shows an overview of the NFT state, which autoban uses to track IP adresses.
    Addresses can also be added or deleted.

  USAGE
    autoban [SUBCOMMAND]
      It is sufficeint to only give the first character of the subcommand.
      If no subcommand is given use "show".

  SUBCOMMAND
    blacklist <saddr>    Add <saddr> to the blacklist set
    blacklist <set>      Add all saddr in <set> to the blacklist set
    delete    <saddr>    Delete <saddr> from all sets
    delete    all        Delete all saddr from all sets
    help                 Print this text
    jail      <saddr>    Add <saddr> to the jail and parole sets
    jail      <set>      Add all saddr in <set> to the jail and parole sets
    list                 List element arrays, used for debugging
    show                 Show overview of the NFT state
    whitelist <saddr>    Add <saddr> to the whitelist set
    whitelist <set>      Add all saddr in <set> to the whitelist set

  EXAMPLES
    autoban blacklist 77.247.110.24 jail 62.210.151.21
    autoban d all


HELP_MESSAGE;

/*------------------------------------------------------------------------------
 Initiate logging and load dependencies.
*/
openlog("autoban", LOG_PID | LOG_PERROR, LOG_LOCAL0);
require_once 'error.inc';
require_once 'autoban.class.inc';

/*------------------------------------------------------------------------------
 Create class objects and set log level.
*/
$ban = new Autoban();

/*--------------------------------------------------------------------------
Add elements $addr to NFT set $set
@param  string $set eg "blacklist"
@param  array of strings $args eg ["23.94.144.50", "jail"]
@return boolean false if unable to add element else true
*/
function add($set, $args) {
	global $ban;
	$timeout = $ban->configtime($set);
	foreach ($args as $arg) {
		if (in_array($arg,Autoban::NFT_SETS)) {
			$addrs = array_keys($ban->list($arg));
			foreach ($addrs as $addr) $ban->add($set, $addr, $timeout);
		} else {
			$ban->add($set, $arg, $timeout, true);
		}
	}
}
/*--------------------------------------------------------------------------
Delete elements $args from NFT sets $sets
@param  array of strings $sets eg ["blacklist"]
@param  array of strings $args eg ["23.94.144.50", "all"]
@return boolean false if unable to delete element else true
*/
function del($sets, $args) {
	global $ban;
	foreach ($args as $arg) {
		foreach ($sets as $set) {
			if ($arg === 'all') {
				$ban->del($set);
			} else {
				$ban->del($set, $arg, true);
			}
		}
	}
}
/*------------------------------------------------------------------------------
 Start code execution.
*/
$subcmd=@$argv[1];
unset($argv[0],$argv[1]);
switch (@$subcmd[0]) {
	case 'b':
		add('blacklist', @$argv);
		break;
	case 'd':
		del(Autoban::NFT_SETS,@$argv);
		break;
	case 'j':
		add('jail', @$argv);
		add('parole', @$argv);
		break;
	case 'l':
		foreach (Autoban::NFT_SETS as $set) var_dump($ban->list($set));
		break;
	case '':
	case 's':
		$ban->show();
		break;
	case 'w':
		add('whitelist', @$argv);
		break;
	case 'h':
	default:
		print $HELP_MESSAGE;
		break;
}
?>
