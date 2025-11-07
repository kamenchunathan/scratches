#include <print>
#include <string>
#include <vector>

#include "Printable_erasure.hpp"

// Define a few structs that implement the Printable concept
struct Dog {
    void print() {
        std::println("Woof!");
    }
    std::string toString() {
        return "Dog";
    }
};

struct Cat {
    void print() {
        std::println("Meow!");
    }
    std::string toString() {
        return "Cat";
    }
};

struct Person {
    std::string name;
    void print() {
        std::println("My name is {}", name);
    }
    std::string toString() {
        return name;
    }
};

int main() {
    // Create a dynamic list of Printable objects
    // std::vector<type_erasure::AnyPrintable> printables;
    // printables.emplace_back(Dog{});
    // printables.emplace_back(Cat{});
    // printables.emplace_back(Person{"John"});

    // Iterate over the list and call the methods
    // for (auto& p : printables) {
    //     p.print();
    //     std::println("  - toString() -> \"{}\"", p.toString());
    // }
}
