#!/bin/bash
if [$# -eq 0 ]; then
    echo "use: $0 num1 num2 ..."
    exit 1
fi

sum=0
count=$#

for num in "$@"; do
    sum=$((sum + num))
done

average=$(echo "scale=2; $sum / $count" | bc)

echo "amount: $count"
echo "average: $average"

