CC=gcc

all: cimin

cimin:
	$(CC) cimin.c
	./a.out -i balance/testcases/fail -m error -o balance_output.txt ./balance/balance
	./a.out -i jsmn/testcases/crash.json -m AddressSanitizer: heap-buffer-overflow -o jsmn_output.txt ./jsmn/jsondump
	./a.out -i libxml2/testcases/crash.xml -m SEGV on unknown address -o libxml_output.txt ./libxml2/xmllint --recover --postvalid -

clean:
	rm -f cimin
	rm -f jsmn_output.txt
	rm -f balance_output.txt
	rm -f libxml_output.txt
