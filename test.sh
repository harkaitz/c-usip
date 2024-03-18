#!/bin/sh
USIP="$(dirname "$0")/usip"

## Example SIP message.
cat > /tmp/example-input <<EOF
INVITE sip:bob SIP/1.1
From           : sip:alice
Content-Length : 10

123456789
Rest ignored.
EOF
print_run() {
    echo "Running: $*"
    "$@"
}


case "$1" in
    forge) ## Generate a sip message.
        print_run $USIP -M -1 INVITE -2 sip:bob -3 SIP/1.1 -a "From:sip:alice" -p <<-EOF
	123456789
	EOF
        ;;
    modify-message)
        echo "Message: ========================="
        cat /tmp/example-input
        echo "=================================="
        print_run $USIP -Mr -i "Route: putxu.com" -a "from: sip:turuta" < /tmp/example-input
        ;;
    get-content)
        echo "Message: ========================="
        cat /tmp/example-input
        echo "=================================="
        print_run $USIP -Pr < /tmp/example-input
        ;;
    get-parameters)
        echo "Message: ========================="
        cat /tmp/example-input
        echo "=================================="
        print_run $USIP -r -G From -G To -G 1 -G 2 -G 3 < /tmp/example-input
        ;;
    *)
        echo "forge          : Test forging a message. "
        echo "modify-message : Parse message, modify and forge again."
        echo "get-content    : Print content."
        echo "get-parameters : Print parameters."
        ;;
esac

