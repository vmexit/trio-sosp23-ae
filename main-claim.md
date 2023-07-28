
### 1. Single thread performance (Sec 6.2)

**Claims**: 
* For small data operations, ArckFS outperforms OdinFS due to direct NVM access and performs similarly to others. 

* For large data operations, ArckFS outperforms OdinFS due to direct NVM access and others due to access delegation. 

* For metadata operations, ArckFS outperforms others due to direct NVM access and efficient data structure design. 


**Expected results**: 
* In Figure 5 (a), ArckFS outperforms OdinFS and performs similarly to others.

* In Figure 5 (b), ArckFS outperforms others.

* In Figure 5 (c) and (d), ArckFS outperforms others. 


### 2. Data operation performance (Sec 6.3)

**Claims**: 

* ArckFS scales NVM throughput with respect to core counts due to the use of access delegation. 

* With small access sizes, ArckFS outperforms after concurrent accesses or remote NVM access cause NVM performance collapses. 

* With large access sizes, ArckFS always outperforms. 

* ArckFS outperforms OdinFS due to direct NVM access. 


**Expected results**: 
* In Figure 6 (a), before 56 threads, ArckFS performs similarly to or worse than other file systems. After 56 threads, ArckFS starts to outperform. 

* In Figure 6 (a), ArckFS always outperform OdinFS. 

* In Figure 6 (b), before 16 threads, ArckFS performs similarly to or worse than other file systems. After 16 threads, ArckFS starts to outperform. 

* In Figure 6 (b), ArckFS always outperform OdinFS. 

* In Figure 6 (c) and (d), ArckFS always outperform other file systems. 

* In Figure 6 (c) and (d), ArckFS always outperform OdinFS until reaching maximal NVM bandwidth. 

### 3. Scalability (Sec 6.4)

**Claims**: 

* ArckFS scales well for common metadata operations 

**Expected results**: 

* In Figure 7, ArckFS scales better than evaluated file systems. 


### 4. Macrobenchmarks (Sec 6.6)

**Claims**: 

* For data-intensive workloads: Filebench and Webserver, ArckFS significantly outperforms other evaluated file systems (due to access delegation) and 

* For read-intensive workloads: Webserver and Videoserver, Odinfs performs similarly to ext4-raid0 and outperforms all other evaluated file systems. 

* For metadata-intensive workloads: Webproxy and Varmail, ArckFS outperforms other file systems. 

**Expected results**: 

* Please verify the above claims in Figure 9. 


### 5. Customized file systems (Sec 6.6)

**Claims**: 

* Customized file systems (KVFS and FPFS) further outperform ArckFS. 

**Expected results**: 

* Please verify the above claims in Figure 10. 

### Notes

* We evaluated ArckFS on an 8 socket, 224 core machines. For machines with fewer sockets or cores, the concurrent access and NVM NUMA impact are not as severe. The aggregated NVM bandwidth is also smaller. Hence, the results with ArckFS may not be as good as we present in the paper, but we believe the overall trends should be similar. 

