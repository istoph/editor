#!/bin/bash
# Here we generate random text to test stdin.
# ./generate-following-input.sh | chr - -f

# Function to generate a random string of the specified length
generate_random_string() {
    local length=$1
    tr -c -d 'a-z0-9 ' </dev/urandom | head -c "$length"
    echo
}

# Number of strings you want to generate
num_strings=1000

for ((i=1; i<=$num_strings; i++)); do
    # Choose a random length between 1 and 200
    length=$((RANDOM % 200 + 1))

    # Generate a random string
    random_string=$(generate_random_string $length)

    # Output the generated string
    echo "String $i: $random_string"

    sleep .2
done

