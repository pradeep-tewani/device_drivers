#!/bin/bash
### default variables
tracing=/sys/kernel/debug/tracing
flock=/var/tmp/.ftrace-lock
bufsize_kb=4096
opt_duration=0; duration=; opt_name=0; name=; opt_pid=0; pid=; ftext=
opt_start=0; opt_end=0; opt_device=0; device=; opt_iotype=0; iotype=
opt_queue=0
trap ':' INT QUIT TERM PIPE HUP	# sends execution to end tracing section

function usage {
	cat <<-END >&2
	USAGE: iosnoop [-hQst] [-d device] [-i iotype] [-p PID] [-n name]
	               [duration]
	                 -d device       # device string (eg, "202,1)
	                 -i iotype       # match type (eg, '*R*' for all reads)
	                 -n name         # process name to match on I/O issue
	                 -p PID          # PID to match on I/O issue
	                 -Q              # use queue insert as start time
	                 -s              # include start time of I/O (s)
	                 -t              # include completion time of I/O (s)
	                 -h              # this usage message
	                 duration        # duration seconds, and use buffers
	  eg,
	       iosnoop                   # watch block I/O live (unbuffered)
	       iosnoop 1                 # trace 1 sec (buffered)
	       iosnoop -Q                # include queueing time in LATms
	       iosnoop -ts               # include start and end timestamps
	       iosnoop -i '*R*'          # trace reads
	       iosnoop -p 91             # show I/O issued when PID 91 is on-CPU
	       iosnoop -Qp 91            # show I/O queued by PID 91, queue time
	See the man page and example file for more info.
END
	exit
}

function warn {
	if ! eval "$@"; then
		echo >&2 "WARNING: command failed \"$@\""
	fi
}

function end {
	# disable tracing
	echo 2>/dev/null
	echo "Ending tracing..." 2>/dev/null
	cd $tracing
	warn "echo 0 > events/block/$b_start/enable"
	warn "echo 0 > events/block/block_rq_complete/enable"
	if (( opt_device || opt_iotype || opt_pid )); then
		warn "echo 0 > events/block/$b_start/filter"
		warn "echo 0 > events/block/block_rq_complete/filter"
	fi
	warn "echo > trace"
	(( wroteflock )) && warn "rm $flock"
}

function die {
	echo >&2 "$@"
	exit 1
}

function edie {
	# die with a quiet end()
	echo >&2 "$@"
	exec >/dev/null 2>&1
	end
	exit 1
}

### process options
while getopts d:hi:n:p:Qst opt
do
	case $opt in
	d)	opt_device=1; device=$OPTARG ;;
	i)	opt_iotype=1; iotype=$OPTARG ;;
	n)	opt_name=1; name=$OPTARG ;;
	p)	opt_pid=1; pid=$OPTARG ;;
	Q)	opt_queue=1 ;;
	s)	opt_start=1 ;;
	t)	opt_end=1 ;;
	h|?)	usage ;;
	esac
done
shift $(( $OPTIND - 1 ))
if (( $# )); then
	opt_duration=1
	duration=$1
	shift
fi
if (( opt_device )); then
	major=${device%,*}
	minor=${device#*,}
	dev=$(( (major << 20) + minor ))
fi

### option logic
(( opt_pid && opt_name )) && die "ERROR: use either -p or -n."
(( opt_pid )) && ftext=" issued by PID $pid"
(( opt_name )) && ftext=" issued by process name \"$name\""
if (( opt_duration )); then
	echo "Tracing block I/O$ftext for $duration seconds (buffered)..."
else
	echo "Tracing block I/O$ftext. Ctrl-C to end."
fi
if (( opt_queue )); then
	b_start=block_rq_insert
else
	b_start=block_rq_issue
fi

### select awk
(( opt_duration )) && use=mawk || use=gawk	# workaround for mawk fflush()
[[ -x /usr/bin/$use ]] && awk=$use || awk=awk
wroteflock=1

### check permissions
cd $tracing || die "ERROR: accessing tracing. Root user? Kernel has FTRACE?
    debugfs mounted? (mount -t debugfs debugfs /sys/kernel/debug)"

### ftrace lock
[[ -e $flock ]] && die "ERROR: ftrace may be in use by PID $(cat $flock) $flock"
echo $$ > $flock || die "ERROR: unable to write $flock."

### setup and begin tracing
echo nop > current_tracer
warn "echo $bufsize_kb > buffer_size_kb"
filter=
if (( opt_iotype )); then
	filter="rwbs ~ \"$iotype\""
fi
if (( opt_device )); then
	[[ "$filter" != "" ]] && filter="$filter && "
	filter="${filter}dev == $dev"
fi
filter_i=$filter
if (( opt_pid )); then
	[[ "$filter_i" != "" ]] && filter_i="$filter_i && "
	filter_i="${filter_i}common_pid == $pid"
	[[ "$filter" == "" ]] && filter=0
fi
if (( opt_iotype || opt_device || opt_pid )); then
	if ! echo "$filter_i" > events/block/$b_start/filter || \
	    ! echo "$filter" > events/block/block_rq_complete/filter
	then
		edie "ERROR: setting -d or -t filter. Exiting."
	fi
fi
if ! echo 1 > events/block/$b_start/enable || \
    ! echo 1 > events/block/block_rq_complete/enable; then
	edie "ERROR: enabling block I/O tracepoints. Exiting."
fi
(( opt_start )) && printf "%-15s " "STARTs"
(( opt_end )) && printf "%-15s " "ENDs"
printf "%-12.12s %-6s %-4s %-8s %-12s %-6s %8s\n" \
    "COMM" "PID" "TYPE" "DEV" "BLOCK" "BYTES" "LATms"

#
# Determine output format. It may be one of the following (newest first):
#           TASK-PID   CPU#  ||||    TIMESTAMP  FUNCTION
#           TASK-PID    CPU#    TIMESTAMP  FUNCTION
# To differentiate between them, the number of header fields is counted,
# and an offset set, to skip the extra column when needed.
#
offset=$($awk 'BEGIN { o = 0; }
	$1 == "#" && $2 ~ /TASK/ && NF == 6 { o = 1; }
	$2 ~ /TASK/ { print o; exit }' trace)

### print trace buffer
warn "echo > trace"
( if (( opt_duration )); then
	# wait then dump buffer
	sleep $duration
	cat trace
else
	# print buffer live
	cat trace_pipe
fi ) | $awk -v o=$offset -v opt_name=$opt_name -v name=$name \
    -v opt_duration=$opt_duration -v opt_start=$opt_start -v opt_end=$opt_end \
    -v b_start=$b_start '
	# common fields
	$1 != "#" {
		# task name can contain dashes
		comm = pid = $1
		sub(/-[0-9][0-9]*/, "", comm)
		sub(/.*-/, "", pid)
		time = $(3+o); sub(":", "", time)
		dev = $(5+o)
	}
	# block I/O request
	$1 != "#" && $0 ~ b_start {
		if (opt_name && match(comm, name) == 0)
			next
		#
		# example: (fields1..4+o) 202,1 W 0 () 12862264 + 8 [tar]
		# The cmd field "()" might contain multiple words (hex),
		# hence stepping from the right (NF-3).
		#
		loc = $(NF-3)
		starts[dev, loc] = time
		comms[dev, loc] = comm
		pids[dev, loc] = pid
		next
	}
	# block I/O completion
	$1 != "#" && $0 ~ /rq_complete/ {
		#
		# example: (fields1..4+o) 202,1 W () 12862256 + 8 [0]
		#
		dir = $(6+o)
		loc = $(NF-3)
		nsec = $(NF-1)
		if (starts[dev, loc] > 0) {
			latency = sprintf("%.2f",
			    1000 * (time - starts[dev, loc]))
			comm = comms[dev, loc]
			pid = pids[dev, loc]
			if (opt_start)
				printf "%-15s ", starts[dev, loc]
			if (opt_end)
				printf "%-15s ", time
			printf "%-12.12s %-6s %-4s %-8s %-12s %-6s %8s\n",
			    comm, pid, dir, dev, loc, nsec * 512, latency
			if (!opt_duration)
				fflush()
			delete starts[dev, loc]
			delete comms[dev, loc]
			delete pids[dev, loc]
		}
		next
	}
	$0 ~ /LOST.*EVENTS/ { print "WARNING: " $0 > "/dev/stderr" }
'

### end tracing
end
