#!/bin/bash
# Used for testing
#

source=${1-+15017122661}
destexten=${2-+15558675310}
message="${3-Local time is $(date)}"

astspooldir=/var/spool/asterisk/outgoing
message_context=dp_entry_text_in
exten_context=dp_entry_answer

maxretry=100
retryint=30
d_unique=$(date +%s)
d_friendly=$(date +%T_%D)
myrandom=$[ ( $RANDOM % 1000 )  + 1 ]

filename="$destexten-$d_unique.$myrandom.call"

cat <<-!cat > $astspooldir/$filename
	Channel: Local/${destexten}@${exten_context}
	CallerID: $source
	Maxretries: $maxretry
	RetryTime: $retryint
	Context: $message_context
	Extension: $destexten
	Priority: 1
	Setvar: MESSAGE(body)=$message
	Setvar: MESSAGE(to)=$destexten
	Setvar: MESSAGE(from)=$source
!cat
