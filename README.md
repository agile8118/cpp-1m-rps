# Overview

This is the C++ version of this Node.js app that we used in the [Handling 1 Million Requests per Second Video](https://youtu.be/W4EwfEU8CGA).

With Node.js, in the patch route that sends a 30KB response, we hit a limit on the AWS C8gn.48xlarge instance, and couldn't get to 1 million requests per second.

So instead of adding more servers, or way more CPU cores, I ended up rewriting the patch route in C++, using Drogon and RapidJSON (some of the fastest libraries in the world), and then we hit the 1 million RPS milestone.

This way, the C++ implementation surpassed my Node.js, Go, and Java implementations and managed to achieve the highest performance and handle the route at 1 million requests per second, moving across 40 GB (320 Gb) per second over the network without breaking a sweat.

## Why C++ is Faster Here?

There are 3 primary reasons why:

1- **Native language benefits.** Languages like C, C++ and Rust are generally faster than higher-level languages for CPU-bound operations.

2- **Faster JSON parsing.** While V8 in August 2025 announced [here](https://v8.dev/blog/json-stringify) that JSON.stringify is now up to 2 times faster, RapidJSON is still faster and is one of the fastest JSON parsers in the world.
Without RapidJSON, the C++ code with the default Drogon JSON parser was actually 4 times slower than the Node.js version.
[Simdjson](https://github.com/simdjson/simdjson) library though, claims to be 4 times faster than RapidJSON, so it'd be interesting to also try this one and see the results. I stopped optimizing the code once we reached 1 million RPS since that was our goal.

3- **We can parallelize more.** Our server was a c8gn.48xlarge instance with 192 CPU cores. Based on the mathematically proven Amdahl’s Law which is as follows:

> Amdahl’s Law:
> 
> $$\text{Speed up} = \frac{1}{(1 - p) + \frac{p}{\text{core count}}}$$
> 
> _P is the fraction of the program that is parallelizable._

We can infer that the C++ Drogon threading version will be much faster than the Node.js cluster version. In Node.js, the parent process acts as a single-threaded dispatcher. It must accept every incoming connection and hand it off to a worker through inter-process communication ([read more here](https://nodejs.org/api/cluster.html#how-it-works)). For that reason, let's say that we can parallelize 95% of our program. If we plug the numbers into the equation we get:

> Amdahl’s Law For Node.js:
> 
> $$\text{Speed up}=\frac{1}{(1 - 0.95) + \frac{0.95}{\text{192}}}\approx 18.20$$

This means that if we can parallelize 95% of our program, we can only achieve around 18x performance boost if we have 192 CPU cores. If we change the core count to a whopping 700 cores, the speed up will only be 19.5. So adding more cores won't linearly improve performance if our parallelizable factor isn't very close to 1.

**BUT...**

In C++, Drogon uses a native multi-threaded event loop which allows almost every stage of a request to happen in parallel across all cores without a single bottleneck process. For that reason, let's say that we can parallelize 99% of our program. If we do the math again:

> Amdahl’s Law For C++ with Drogon:
> 
> $$\text{Speed up}=\frac{1}{(1 - 0.99) + \frac{0.99}{\text{192}}}\approx 66$$

So we can speed up the C++ code dramatically more simply because we can parallelize just a few more percent of our application. The difference will be negligible if we only have a few cores, which is common for many real-world applications, but when we move to extreme environments and the busiest routes in the world, this difference gets significantly more noticeable.
