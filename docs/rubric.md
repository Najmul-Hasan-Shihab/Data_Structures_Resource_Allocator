# Project Rubric: Emergency Resource Allocator

This rubric is tuned to the provided syllabus and the final exam evaluation scheme.

## Syllabus Alignment

The project aligns most strongly with:

- Queue and priority queues: emergency triage, waiting queue, serve-next workflow.
- Heap: max-heap ordering, insert, extract-max, priority update.
- Hashing: resource lookup, assignment, and release by resource ID.
- Complexity analysis: insert, extract, update, lookup, and queue operations.
- Optional extension space: linked structures in logging, graph-based routing, and sorting/search comparisons in documentation.

It also gives room to mention broader syllabus ideas in the report, such as:

- Algorithmic notation and time-space tradeoff.
- Searching and sorting comparisons.
- Trees and graphs as future extensions.

## Evaluation Rubric

### 1. Relevance to Syllabus & CLOs - 5 marks

- 5: Excellent coverage of multiple DS topics with clear linkage to queue, heap, hashing, and complexity.
- 3: Good coverage of the main topics.
- 2: Basic coverage with limited cross-topic linkage.
- 0-1: Poor alignment with the syllabus.

### 2. Technical Implementation - 5 marks

- 5: Fully functional, correct, efficient, and uses data structures appropriately.
- 3: Works with minor issues.
- 2: Basic working version.
- 0-1: Major bugs or incomplete implementation.

### 3. Innovation & Real-World Value - 3 marks

- 5-level quality: Strong practical relevance, realistic triage flow, and an impactful use case.
- 3: Good real-world application.
- 2: Standard application with limited novelty.
- 0-1: Weak or unclear real-world connection.

Suggested evidence:

- Emergency triage scenario.
- Resource shortage handling.
- Waiting queue fallback.
- Optional future support for multi-hospital dispatch.

### 4. Analysis & Documentation - 3 marks

- 5-level quality: Thorough complexity analysis, strong report, diagrams, examples, and test cases.
- 3: Good documentation.
- 2: Basic report.
- 0-1: Missing or poor documentation.

Suggested report sections:

- Problem statement and motivation.
- Data structure choice.
- Operation-wise complexity table.
- Limitations and future work.
- Sample run screenshots and test cases.

### 5. Presentation / Demo & Viva - 4 marks

- 5-level quality: Clear live demo, smooth explanation, and confident handling of questions.
- 3: Good demo.
- 2: Basic demo.
- 0-1: Weak or missing demo.

Suggested viva flow:

1. Show resource setup.
2. Add emergencies with different severities.
3. Demonstrate heap order.
4. Serve the highest-priority case.
5. Increase priority of a waiting emergency.
6. Add a resource and show waiting-case recovery.
7. Explain complexity and why heap is better than FIFO.

## Final Score Guide

- 20-18: Excellent submission.
- 17-15: Strong submission with minor gaps.
- 14-12: Acceptable but needs clearer explanation or polish.
- Below 12: Needs major improvement in implementation or presentation.

## Project-Specific Notes for the Viva

- Queue is the core syllabus match, but the heap gives the project its strong technical value.
- Hashing makes resource lookup fast and easy to justify in complexity terms.
- The system can be extended to graphs and routing if the examiner asks about future growth.
- The documentation should explicitly contrast the heap approach with a sorted array or plain FIFO queue.