#!/bin/bash

for filename in `find . | egrep '\.o'`;
do
		  gcov-5 $filename > /dev/null;
done

bash <(curl -s https://codecov.io/bash);
