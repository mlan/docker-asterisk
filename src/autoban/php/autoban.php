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
      If no subcommand is given use "show".

  SUBCOMMAND
    add <dsets> = <ssets> <addrs>   Add to <dsets>, <addrs> and/or
                                    addrs from <ssets>.
    del <dsets> = <ssets> <addrs>   Delete from <dsets>, <addrs> and/or
                                    addrs from <ssets>.
    list <sets>                     List addrs from <sets>.
    help                            Print this text.
    show                            Show overview of the NFT state.

  EXAMPLES
    Blacklist 77.247.110.24 and 62.210.151.21 and all addresses from jail
      autoban add blacklist = 77.247.110.24 jail 62.210.151.21

    Add all addresses in the watch set to the jail and parole sets
      autoban add jail parole = watch

    Delete 37.49.230.37 and all addresses in blacklist from jail parole
      autoban del jail parole = 37.49.230.37 blacklist

    Delete 45.143.220.72 from all sets
      autoban del all = 45.143.220.72

    Delete all addresses from all sets
      autoban del all = all


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
@param  array of strings $args eg ["blacklist", "=", "77.247.110.24", "jail"]
@return boolean false if unable to add element else true
*/
function add($args) {
	global $ban;
	parse($args,$dargs,$sargs,'blacklist');
	foreach ($dargs as $dset) {
		$timeout = $ban->configtime($dset);
		$assume_ssets = array_intersect($sargs,Autoban::NFT_SETS);
		$assume_saddrs = array_diff($sargs,Autoban::NFT_SETS);
		foreach ($assume_ssets as $sset) {
			$saddrs = array_keys($ban->list($sset));
			$ban->add_addrs($dset, $saddrs, $timeout);
		}
		$ban->add_addrs($dset, $assume_saddrs, $timeout, true);
	}
	$ban->save();
}

/*--------------------------------------------------------------------------
Delete elements $args from NFT sets $sets
@param  array of strings $args eg ["blacklist", "=", "77.247.110.24", "jail"]
@return boolean false if unable to delete element else true
*/
function del($args) {
	global $ban;
	parse($args,$dargs,$sargs,'all');
	if (array_search('all', $dargs) !== false) $dargs = Autoban::NFT_SETS;
	foreach ($dargs as $dset) {
		$assume_ssets = array_intersect($sargs,Autoban::NFT_SETS);
		foreach ($assume_ssets as $sset) {
			$saddrs = array_keys($ban->list($sset));
			$ban->del_addrs($dset, $saddrs);
		}
		$assume_saddrs = array_diff($sargs,Autoban::NFT_SETS);
		$ban->del_addrs($dset, $assume_saddrs);
	}
	$ban->save();
}

/*--------------------------------------------------------------------------
List elements in NFT sets $sets
@param  array of strings $args eg ["blacklist", "jail"]
@return void
*/
function ls($args) {
	global $ban;
	if (empty($args) || array_search('all', $args) !== false)
		$args = Autoban::NFT_SETS;
	foreach ($args as $set)
		if (count($args) === 1)
			printf("%s\n", implode(' ',array_keys($ban->list($set))));
		else
			printf("%s: %s\n", $set, implode(' ',array_keys($ban->list($set))));
}

/*--------------------------------------------------------------------------
Separates argument in to $dargs and $sargs using the separator
@param  array of strings $args eg ["blacklist", "=", "77.247.110.24", "jail"]
@param  array of strings $dargs eg ["blacklist"]
@param  array of strings $sargs eg ["77.247.110.24", "jail"]
@return void
*/
function parse($args, &$dargs, &$sargs, $default = 'all', $separators = ':+-=') {
	$left = []; $right = [];
	foreach ($args as $arg) {
		if (strlen($arg) === 1 && strstr($separators, $arg) !== false) {
			$mid = $arg;
		} else {
			if (empty($mid)) {
				array_push($left, $arg);
			} else {
				array_push($right, $arg);
			}
		}
	}
	if (empty($right)) {
		$dargs = [$default];
		$sargs = $left;
	} else {
		$dargs = $left;
		$sargs = $right;
	}
}
/*------------------------------------------------------------------------------
 Start code execution.
 Scrape off command and sub-command and pass the rest of the arguments.
*/
#$ban->debug = true;
$subcmd=@$argv[1];
unset($argv[0],$argv[1]);
#if(!empty($subcmd))
#	trigger_error(sprintf('Running %s %s', $subcmd, implode(' ',$argv)),
#		E_USER_NOTICE);
switch (@$subcmd) {
	case 'add':
	case 'a':
		add(@$argv);
		break;
	case 'delete':
	case 'del':
	case 'd':
		del(@$argv);
		break;
	case 'list':
	case 'ls':
	case 'l':
		ls(@$argv);
		break;
	case 'show':
	case 's':
	case '':
		$ban->show();
		break;
	case 'help':
	default:
		print $HELP_MESSAGE;
		break;
}
?>
