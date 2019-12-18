#  AutoBan

AutoBan is an intrusion detection and prevention system which is built in the `mlan/asterisk` container. The intrusion detection is achieved by Asterisk itself. Asterisk generates security events which AutoBan listens to on the AMI interface. When security events occurs AutoBan start to watch the source IP address. Intrusion prevention is achieved by AutoBan asking the Linux kernel firewall [nftables](https://netfilter.org/projects/nftables/) to drop packages from offending source IP addresses.

## Operation

Asterisk generates security events which AutoBan listens to on the AMI interface. When one of the security events; `InvalidAccountID`, `InvalidPassword`, `ChallengeResponseFailed`, or `FailedACL` occurs AutoBan start to watch the source IP address for `watchtime` seconds. If more than `maxcount` security events occurs within this time, all packages from the source IP address is dropped for `jailtime` seconds.

When the `jailtime` expires packages are again accepted from the source IP address, but for an additional `watchtime` seconds this address is on "parole". Should a security event be detected (from this address) during the parole period it is immediately blocked again, and for a progressively longer time. This progression is configured by `repeatmult`, which determines how many times longer the address is blocked. If no security event is detected from this address during its parole time the IP is no longer being watched.

To illustrate, first assume `jailtime=20m` and `repeatmult=6`, then the IP is blocked 20 minutes the first time, 2 hours the second, 12 hours the third, 3 days the forth and so on.

## Configuration

The function of AutoBan is controlled by two configuration files; `autoban.conf` and `manager.conf`. Additionally, the docker container needs extra capabilities to be able to control networking.

| File name    | Description                                                  |
| ------------ | ------------------------------------------------------------ |
| autoban.conf | Configurations which are unique to the AutoBan service       |
| manager.conf | Read by the Asterisk Manager Interface (AMI), configuring both the server and client(s) |

### Docker, runtime privileges

AutoBan uses [nftables](https://en.wikipedia.org/wiki/Nftables) which does the actual package filtering. The container needs additional [runtime privileges](https://docs.docker.com/v17.12/engine/reference/run/#runtime-privilege-and-linux-capabilities) to be able to do that. Nftables needs the `NET_ADMIN` and `NET_RAW` capabilities to function, which you provide by adding these options to the docker run command `--cap-add=NET_ADMIN --cap-add=NET_RAW`.

### Configuring AutoBan, autoban.conf

AutoBan is activated if there is an `autoban.conf` file and that the parameter `enabled` within is not set to `no`.

| Section     | Key        | Default   | Format                                       | Description                                                  |
| ----------- | ---------- | --------- | -------------------------------------------- | ------------------------------------------------------------ |
| [asmanager] | server     | localhost | ip or fqdn                                   | Here asterisk runs in same container so AMI server address is localhost or 127.0.0.1 |
| [asmanager] | port       | 5038      | integer                                      | AMI server port number, same as port used in manager.conf    |
| [asmanager] | username   | phpagi    | string                                       | AMI client name, same as section [<client>] in manager.conf  |
| [asmanager] | secret     | phpagi    | string                                       | AMI client password, same as secret in manager.conf          |
| [autoban]   | enabled    | true      | boolean                                      | AutoBan is activated when autoban.conf exists and not explicitly disabled here |
| [autoban]   | maxcount   | 10        | integer                                      | Abuse count at which IP will be jailed, that is, its packets will be dropped |
| [autoban]   | watchtime  | 20m       | string, integer followed by unit; d, h, m, s | Time to keep IP under watch in seconds or time string, example: 1d2h3m4s |
| [autoban]   | jailtime   | 20m       | string, integer followed by unit; d, h, m, s | Time to drop packets from IP in seconds or time string example: 1d2h3m4s |
| [autoban]   | repeatmult | 6         | integer or float                             | Repeat offenders get jailtime multiplied by this factor      |

### Configuring the AMI, manager.conf

The AMI interface is configured in `manager.conf`.The table below does only describe to most relevant keys. Please refer to [AMI Configuration](https://wiki.asterisk.org/wiki/display/AST/AMI+v2+Specification#AMIv2Specification-AMIConfiguration), for full details.

| Section    | key      | Default | Format     | Description                                                  |
| ---------- | -------- | ------- | ---------- | ------------------------------------------------------------ |
| [general]  | enabled  | no      | boolean    | Enable AMI server, yes or no                                 |
| [general]  | bindaddr | 0.0.0.0 | ip address | Binds the AMI server to this IP address, for security reasons set this to; 127.0.0.1 |
| [general]  | port     | 5038    | integer    | Port the AMI's TCP server will listen to, same as, port in autoban.conf |
| [<client>] |          |         | string     | Client name which AutoBan uses to authenticate with the AMI server, same as username in autoban.conf |
| [<client>] | secret   |         | string     | Password which AutoBan uses to authenticate with the AMI server, same as secret in autoban.conf |
| [<client>] | read     | all     | string     | A comma delineated list of the allowed class authorizations applied to events, for security reasons limit to;  security |
| [<client>] | write    | all     | string     | A comma delineated list of the allowed class authorizations applied to actions, for security reasons limit to; "" |

### Default configuration

If the Asterisk configuration directory is empty, default configuration files will be copied there at container startup. The ones relevant here are `autoban.conf` and `manager.conf`. For security reasons it is suggested that the `secret` is changed in both files.

`autoban.conf`

```ini
[asmanager]
server     = 127.0.0.1
port       = 5038
username   = autoban
secret     = 6003.438

[autoban]
enabled    = true
maxcount   = 10
watchtime  = 20m
jailtime   = 20m
repeatmult = 6
```
`manager.conf`

```ini
[general]
enabled  = yes
bindaddr = 127.0.0.1
port     = 5038

[autoban]
secret   = 6003.438
read     = security
write    =
```

## Implementation
AutoBan keeps track of which IP addresses are being watched, jailed or on parole by using the names sets and timeout constructs of [nftables](https://wiki.nftables.org/wiki-nftables/index.php/Main_Page). Timeout determines the time an element stays in the set. This mechanism is used to "automatically" keep tack of the watch, jail, and parole times. All state information is kept by nftables, so the AutoBan state can be preserved during container restarts by saving and loading the nftables configuration.

At container startup nftables is configured by loading the file `autoban.nft` which defines the sets; watch, jail, parole, whitelist, and blacklist. Initially these sets does not contain any elements, that is, IP addresses.

The sets; watch, jail and parole use timeout to keep track of how long the IP addresses should stay in each set. So these sets are dynamic. The sets watch and parole are only used for time keeping, so they have no rule attached. Contrarily, all packages from IP source addresses in the set jail will be dropped.  

If you want to permanently whitelist or blacklist source IP addresses, you can add them to the sets; whitelist or blacklist. Packages from source IP addresses in the set whitelist will always be accepted, whereas they will always be dropped if they are in the set blacklist.

## Command line utility, `autoban`

In addition to the AutoBan daemon, `autoband.php` that listens to AMI events and controls the Linux kernel firewall, there is a shell utility `autoban`, that you can use within the container, that helps with managing the NFT state. It can add, delete, white list and black list IP addresses for example.

You can see the `autoban` help message by, from within the container, typing:

```bash
autoban help

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
```

You can watch the status of the `nftables` firewall by, from within the container, typing:

```bash
nft list ruleset
```
