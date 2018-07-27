#!/usr/bin/awk
set ns [new Simulator]

# set colors for connection
$ns color 0 blue
$ns color 1 red


set r_1 [$ns node]
set r_2	[$ns node]

set src_1 [$ns node]
set src_2 [$ns node]

set rcv_1 [$ns node]
set rcv_2 [$ns node]


# set variables
set rat 0
set s_0 0
set s_1 0


set f [open tcp_0.tr w]
$ns trace-all $f
set nf [open tcp_0.nam w]
$ns namtrace-all $nf


#now begin to execute
if { $argc != 2 } {

    puts "\t\tns2.tcl <TCP_fla> <case_num>\t\t"

} else {

    set TCP_fla [lindex $argv 0]
    set case_num [lindex $argv 1]

	  puts "TCP Flavor = $TCP_fla"
	 	puts "Case = $case_num"

}


#execute with given values in command line
$ns duplex-link $r_1 $r_2 1Mb 5ms DropTail

$ns queue-limit $r_1 $r_2 10

# choose case number
if { $case_num == 1 } {

    $ns duplex-link $r_2 $rcv_2 10Mb 12.5ms DropTail
	  $ns duplex-link $r_2 $rcv_1 10Mb 5ms DropTail

    $ns duplex-link $src_2 $r_1 10Mb 12.5ms DropTail
		$ns duplex-link $src_1 $r_1 10Mb 5ms DropTail

} elseif { $case_num == 2 } {

	  $ns duplex-link $r_2 $rcv_2 10Mb 20ms DropTail
		$ns duplex-link $r_2 $rcv_1 10Mb 5ms DropTail

    $ns duplex-link $src_2 $r_1 10Mb 20ms DropTail
		$ns duplex-link $src_1 $r_1 10Mb 5ms DropTail

} elseif { $case_num == 3 } {

	  $ns duplex-link $r_2 $rcv_2 10Mb 27.5ms DropTail
		$ns duplex-link $r_2 $rcv_1 10Mb 5ms DropTail

	  $ns duplex-link $src_2 $r_1 10Mb 27.5ms DropTail
		$ns duplex-link $src_1 $r_1 10Mb 5ms DropTail

} else {

	  puts "Unknown Case"

}


#give the topology layout for the network
$ns duplex-link-op $r_1 $r_2 orient right

$ns duplex-link-op $rcv_2 $r_2 orient left-up
$ns duplex-link-op $rcv_1 $r_2 orient left-down

$ns duplex-link-op $src_2 $r_1 orient right-up
$ns duplex-link-op $src_1 $r_1 orient right-down


$ns duplex-link-op $r_1 $r_2 queuePos 0.5

$ns duplex-link-op $r_2 $rcv_2 queuePos 0.5
$ns duplex-link-op $r_2 $rcv_1 queuePos 0.5


#different routing algorithm
if { $TCP_fla == "VEGAS" } {

		set tcp_0 [new Agent/TCP/Vegas]
		set tcp_1 [new Agent/TCP/Vegas]

    puts "VEGAS"

} elseif { $TCP_fla == "SACK" } {

		set tcp_0 [new Agent/TCP/Sack1]
		set tcp_1 [new Agent/TCP/Sack1]

    puts "SACK"

} else {

    puts "Unknown TCP flavor"

}


$tcp_0 set class_ 0
$tcp_1 set class_ 1

#tcp
set sink0 [new Agent/TCPSink]

$ns attach-agent $rcv_1 $sink0
$ns attach-agent $src_1 $tcp_0


$ns connect $tcp_0 $sink0


set sink1 [new Agent/TCPSink]

$ns attach-agent $rcv_2 $sink1
$ns attach-agent $src_2 $tcp_1


$ns connect $tcp_1 $sink1


#ftp
set ftp_0 [new Application/FTP]
set ftp_1 [new Application/FTP]


$ftp_0 attach-agent $tcp_0
$ftp_1 attach-agent $tcp_1


$ns at 0.0 "record"

$ns at 1.0 "$ftp_0 start"
$ns at 1.0 "$ftp_1 start"

$ns at 401.0 "$ftp_1 stop"
$ns at 401.0 "$ftp_0 stop"

$ns at 402.0 "finish"


#give the execution record on the command line
proc record {} {

    global s_0 s_1 sink0 sink1 rat

	  set ns [Simulator instance]
	  set t 1.0

	  set b_0 [$sink0 set bytes_]
	  set b_1 [$sink1 set bytes_]

	  set now [$ns now]

  	if { $now < 400 && $now >= 100 }	{

		    set s_0 [expr $s_0 + $b_0 / $t * 8 / 1000000]
		    set s_1 [expr $s_1 + $b_1 / $t * 8 / 1000000]

	  }

	  if { $now == 400 } {

		    puts "average throughput of link1  = [expr $s_0 * $t / 300] Mbps"
		    puts "average throughput of link2  = [expr $s_1 * $t / 300] Mbps"
		    set rat [expr $s_0 / $s_1]
		    puts "ratio of link1/link2 = [expr $rat]"

	  }

	  $sink0 set bytes_ 0
	  $sink1 set bytes_ 0

	  $ns at [expr $now + $t] "record"

}


proc finish {} {

	  global ns f nf
	  global ns data

	  $ns flush-trace
	  close $nf
	  close $f

	  puts "nam running..."
	  exec nam tcp_0.nam &

	  exit 0

}


$ns run
