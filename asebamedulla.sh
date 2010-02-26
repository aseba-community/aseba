#!/bin/sh

case "$1" in
        start)
                echo "starting asebamedulla"
                start-stop-daemon -q -S -b -x asebamedulla -- --system "ser:device=/dev/ttymxc4;fc=hard;baud=921600"
                touch /var/lock/subsys/asebamedulla
                ;;
        stop)
                start-stop-daemon -K -x asebamedulla
		rm -rf /var/lock/subsys/asebamedulla
                ;;
        restart)
                $0 stop
                sleep 1
                $0 start
                ;;
        status)
		#if pgrep -x asebamedulla &> /dev/null; then echo "running"; else echo "stopped"; fi
		if pgrep -x asebamedulla &> /dev/null
		then
			echo "asebamedulla running"
		else
			echo "asebamedulla stopped"
		fi
                ;;
        *)
                echo "usage: asebamedulla {start|stop|restart|status}"
                exit 1
esac

exit 0

