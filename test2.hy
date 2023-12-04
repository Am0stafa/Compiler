// Test File: test.hy

// Testing variable declaration and arithmetic operations
let a = 10;
let b = 5;
let sum = a + b; // 15
let diff = a - b; // 5
let product = a * b; // 50
let quotient = a / b; // 2

// Testing boolean literals and equality operator
let isTen = 10 == a; // true
let isFalse = false;
let isTrue = true;

// Testing logical AND and OR operations
let andResult = isTrue && isFalse; // false
let orResult = isTrue || isFalse; // true

// Testing single-line comments
// This is a single-line comment

/* Testing block comments
   This is a block comment
   spanning multiple lines */

// Testing if statements with logical and arithmetic expressions
if (sum == 15) {
    exit(1); // Exit code 1 if sum is 15
}

if (product > 30 && quotient < 3) {
    exit(2); // Exit code 2 if product is greater than 30 AND quotient is less than 3
}

if (isTrue || isFalse) {
    exit(3); // Exit code 3 if isTrue OR isFalse is true
}
