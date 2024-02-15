# TCP Performance Analysis Project

This repository contains the implementation and analysis of TCP Hybla, TCP Westwood+, and TCP YeAH-TCP in a Dumbbell network topology using ns-3 simulator. The project aims to compare the performance of these TCP variants under different scenarios.

## Dumbbell Topology

The network consists of two routers (R1 and R2) connected by a wired link with a bandwidth of 10 Mbps and a delay of 50 ms. Each router is connected to three hosts (H1, H2, H3 for senders, and H4, H5, H6 for receivers) through links with a bandwidth of 100 Mbps and a delay of 20 ms.

## Simulation Parameters

-   **Packet Size:** 1.3 KB
-   **Queue Type:** Drop-tail
-   **Queue Size:** Set according to the bandwidth-delay product
-   **TCP Agents:** H1 uses TCP Hybla, H2 uses TCP Westwood+, and H3 uses TCP YeAH-TCP
-   **Experiment Duration:** Long enough to ensure stable throughput, typically measured in seconds

## Tasks

### 1. Single Flow Analysis

-   Start one flow at a time and analyze throughput over a sufficiently long duration.
-   Plot the evolution of congestion window vs. time.
-   Perform this experiment for each flow attached to all three sending agents.

### 2. Multiple Flows Sharing the Bottleneck

-   Start two additional flows while the first one is in progress.
-   Measure the throughput (in Kbps) of each flow.
-   Plot the throughput and evolution of TCP congestion window for each flow at steady-state.
-   Report the maximum throughput observed for each flow.

### 3. Congestion Loss and Goodput Measurement

-   Measure congestion loss and goodput over the duration of the experiment for each flow.

## Instructions for Running the Simulation

1.  Clone the repository: `git clone https://github.com/sanu0/TCP-Performance-Analysis-Project.git`
2.  Navigate to the project directory: `cd TCP-Performance-Analysis-Project`
3.  Install ns-3 simulator following the official instructions.
4.  Run the simulation script for each scenario.

## Folder Structure

-   **scripts:** Contains ns-3 simulation scripts for different experiments.
-   **results:** Store simulation results, including throughput, congestion window evolution plots, and analysis reports.

## Results and Analysis

Detailed results and analysis can be found in the "project report" pdf file. It provides insights into the observed performance of TCP Hybla, TCP Westwood+, and TCP YeAH-TCP under various conditions. I used gnuplot to plot the graphs from the trace files.

Feel free to explore the code and results to gain a deeper understanding of the project and its outcomes. If you have any questions or suggestions, please open an issue in the repository.

**Happy Simulating!**
