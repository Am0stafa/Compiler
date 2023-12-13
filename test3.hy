// Define a simple add function
let add = function(a, b) {
    let result = a + b;
    return result;
};

// Call the add function
let sum = add(10, 20);

// Define a recursive factorial function
let factorial = function(n) {
    if (n == 0) {
        return 1;
    } else {
        return n * factorial(n - 1);
    }
};

// Call the factorial function
let factResult = factorial(5);

// Define a function that uses nested calls
let compute = function(x, y) {
    let temp = add(x, y);
    return factorial(temp);
};

// Call the function with nested calls
let nestedResult = compute(3, 2);

// Use the exit statement to end the program
exit(0);
