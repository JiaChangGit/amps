# Multiple Parallel Packet Classification (multiple\_parallel\_PC)

## Overview

The **multiple\_parallel\_PC** project implements a high-performance packet classification algorithm.


## References

This implementation is based on the following research papers:

1. KSet

**Y.-K. Chang et al.**, "Efficient Hierarchical Hash-Based Multi-Field Packet Classification With Fast Update for Software Switches," IEEE Access, vol. 13, pp. 28962-28978, 2025. [DOI: 10.1109/ACCESS.2025.3540411]

2. PT

**Z. Liao et al.**, "PT-Tree: A Cascading Prefix Tuple Tree for Packet Classification in Dynamic Scenarios," IEEE/ACM Transactions on Networking, vol. 32, no. 1, pp. 506-519, Feb. 2024. [DOI: 10.1109/TNET.2023.3289029]

3. DBT

**Z. Liao et al.**, "DBTable: Leveraging Discriminative Bitsets for High-Performance Packet Classification," IEEE/ACM Transactions on Networking, vol. 32, no. 6, pp. 5232-5246, Dec. 2024. [DOI: 10.1109/TNET.2024.3452780]


## Files Structure


## How to Compile and Run


### Compilation:


### Execution:



## Performance Evaluation

The classifier evaluates packet classification based on:

- **Construction Time**: Measures the time taken to build the classifier.

- **Classification Throughput**: Evaluates packets per second processed.

- **Memory Usage**: Analyzes the memory footprint of classification structures.

- **Update Efficiency**: Measures insertion and deletion times.


## License

This project is open-source under the MIT License. Contributions and improvements are welcome!

