# C++ Embedded Scripting Language

## Motivation

I started this project when I was at university (in 2017) because I was looking for some challenging programming project that would challenge me for some time.
In the end, by Jason Turners' open source Chaiscript, I decided to implement a scripting language in C++.
See webpage http://chaiscript.com/index.html or git hub repo https://github.com/ChaiScript/ChaiScript

As this was my first attempt to write a scripting language I spent a couple of hours reading Chaiscripts implementation to get an idea of what kind of challenges implementing scripting was going to be. That's why you might find a lot of similarities in the design.

## Goals at the time

My goals at the time of implementing it:
  - Familiarise myself with C++14 features.
  - Explore TDD (Test Driven Development) in a real case scenario.
  - Learn about plain text file parsing.
  - Go for a weakly typed scripting language (because it seemed more challenging to implement).

## Lessons learned

  - Parsing of text files is not that hard, this project gave me a lot of insight in how to approach it.
  - TDD is great for this kind of module, it helped me identify problems early while adding features. Implementing the parser in a way that could be Mocked yield to a better design.
  - Weakly typed languages aren't cool, next time I would go for a strongly typed language.
  - Creating a AST tree of nodes with virtual functions makes things quite slow, next time I would compile the scripts to bytecode.
