# MeerKAT SETI search onboarding notes

## Technical memos:
* [BLUSE overview](https://docs.google.com/document/d/1uj7vAF1FXq7kQcGdi2lr7K2eg98MFW3d3eqsAB2Z3LQ/edit#heading=h.twuqnlahbx18): Overview document of the BLUSE system.
* [DSP plan](https://docs.google.com/document/d/1mrrn3YFABuoYqy0pkphNJYT4j44_slB8VltTEUHSlv0/edit#): Description of the Upchannelization and beamforming schemes 
* [RAW input file format](https://docs.google.com/document/d/1dnye0HHSlVqRXH7rQ7v3wly0qKg-3_9tGJzaTI-76s4/edit#): Description of the RAW input data format
* [MK hashpipe to-do list](https://docs.google.com/document/d/1NrggefvZZ1pxu1ArdtUJGn7RGECHQMxR_JlFeLr0jpc/edit#): The project to-do lists
* [BL Coherent Beamformer](https://docs.google.com/document/d/1Rw49TWaAqymom31Hs9Z4tHTSwquZ71d287gYU8WlrSo/edit#): Compiling and running the coherent beamformer code
* [BL Beamformer Coefficient Phase Computation](https://docs.google.com/document/d/1m228WmeaXnpvqy-y55w6mxyoMrXU1H1rCGy2khO9MUo/edit#): Details on coefficient phase computation with beamformer recipe files
* [CAM interfacing](https://danielczech.github.io/meerkat-diagrams/): Interactive diagrams illustrating the interaction between backend components during a commensal observation. 
* [Automator](https://docs.google.com/document/d/1SjgYNrjrqaFfCKWCjN2TDElBIDFnG_9XAbdMfvHe9GM/edit?usp=sharing): Design notes for initial end-to-end automation of observations. 

## Code bases:
* [hpguppi_daq MeerKAT branch](https://github.com/UCBerkeleySETI/hpguppi_daq/tree/cherry-dev/src): With incoherent beamforming and write out filterbank data.
* [MK DSP](https://github.com/UCBerkeleySETI/MeerKAT_DSP): Earlier versions of the MK DSP, now largely deprecated by the `hpguppi_daq MeerKAT branch` linked above.
* [meerkat-backend-interface](https://github.com/UCBerkeleySETI/meerkat-backend-interface): Comprehensive metadata interface to MeerKAT. See the interactive [diagrams](https://danielczech.github.io/meerkat-diagrams/) ([repository](https://github.com/danielczech/meerkat-diagrams)) illustrating the sequence of events during an observation.   
    - [katcp_server](https://github.com/UCBerkeleySETI/meerkat-backend-interface/blob/python3/meerkat_backend_interface/katcp_server.py): The BLUSE proxy interface for inclusion as a subarray component.  
    - [katportal_server](https://github.com/UCBerkeleySETI/meerkat-backend-interface/blob/python3/meerkat_backend_interface/katportal_server.py): MeerKAT Control and Monitoring sensor interface. 
    - [coordinator](https://github.com/UCBerkeleySETI/meerkat-backend-interface/blob/python3/meerkat_backend_interface/coordinator.py): Delivers appropriate metadata and recording instructions to the processing nodes. Dynamically allocates processing nodes to active subarrays. 
    - [slack_proxy](https://github.com/UCBerkeleySETI/meerkat-backend-interface/blob/python3/meerkat_backend_interface/slack_proxy.py): Provides a Redis-Slack bridge for communication with Redis channels.  
      **Note:** To send a message `<msg>` to the Slack channel `<chan>`, publish a message of the form `<chan>:<msg>` to the `slack-messages` channel 
* commensal-automator:  
    - [Repository](https://github.com/UCBerkeleySETI/commensal-automator): End-to-end automation of commensal observing. 
    - [Design](https://docs.google.com/document/d/1SjgYNrjrqaFfCKWCjN2TDElBIDFnG_9XAbdMfvHe9GM/edit?usp=sharing): Document with notes on the design of the `automator`. 
* [meerkat-monitor](https://github.com/UCBerkeleySETI/meerkat-monitor): General independent monitoring of BLUSE's inclusion in active subarrays. 
* [target-selector](https://github.com/danielczech/targets-minimal): Choosing which targets to observe for a given pointing, taking priorities into account.
* [CAMcorder](https://github.com/danielczech/CAMcorder): Record and replay Redis messages for observation simulation purposes.  
* [meerkat-reconfig](https://github.com/danielczech/meerkat-reconfig): Resend configuration information to processing nodes for development/diagnostic purposes. 
* [RFI flagging]: link pending from Jiapeng
* [hashpipe](http://w.astro.berkeley.edu/~davidm/hashpipe.git/) <- the main code. And here is a link to some [demo](https://github.com/SparkePei/demo1_hashpipe). 
* [rawspec](https://github.com/UCBerkeleySETI/rawspec)
* [FBFUSE Mosaic](https://gitlab.mpifr-bonn.mpg.de/wchen/Beamforming/tree/master/mosaic): FBFUSE software to handle all of the delay calculations as well as PSF modelling for the array and beam tiling. These have to be used in combination with yet another large set of wrapping control codes that include mechanisms to pull the complex gain solutions from SDPs Telstate and apply them to the data prior to beamforming.
* [FBFUSE beamformer](https://github.com/ewanbarr/psrdada_cpp/tree/fbfuse_complex_gain_correction): FBFUSE beamformer for reference


## MK SETI paper publications:
* [The Breakthrough Listen Search for Intelligent Life: MeerKAT Target Selection](https://ui.adsabs.harvard.edu/abs/2021PASP..133f4502C/abstract): by Daniel Czech et al., 2021


## MeerKAT documentations:
* [Key specification](https://skaafrica.atlassian.net/rest/servicedesk/knowledgebase/latest/articles/view/277315585#MeerKATspecifications-Primarybeamcharacteristics): Includie characteristics for the beam shapes
* [Antenna position](https://docs.google.com/spreadsheets/d/1T6bqZBnEXMTFqMFCLs221qOvIOTSLHz_oxbI6RE4TrQ/edit#gid=0) A spreadsheet of antenna positions

## BLUSE data:
* [Obs log](https://docs.google.com/spreadsheets/d/1-wZceD-DDaGydIghOhE9sZC3wICqRAUtR5ny6o0N7l8/edit#gid=1533851989): Log of all data collected
* [Test data on cloud](https://docs.google.com/spreadsheets/d/1qTYAvRcfeIyKA9yUaFO9dC-tLoKSJ_5gwf-bBwYiYX4/edit#gid=0): Info of test data on the cloud
* [Cloud data](https://console.cloud.google.com/storage/browser/blmeerkat_uk?project=dotted-saga-110420&pageState=(%22StorageObjectListTable%22:(%22f%22:%22%255B%255D%22))&prefix=&forceOnObjectsSortingFiltering=false): Test data on the cloud


## Useful demos: 
* [Hashpipe tour](https://drive.google.com/file/d/1s5YR0mGSl7UsBZTndXnzLQ0mrOHN7W6j/view?usp=sharing): by Dave M. given at iSETI weekly on 2020-10-15
* [Hashpipe demos](https://github.com/SparkePei/demo1_hashpipe): Four simple demos from SparkPei


## Previous presentations: 
* [BL MeerKAT slide deck](https://docs.google.com/presentation/d/1tKlvAaVFdGViZfZ6mD9XTiMshtWjChFadIcHzaJMpx8/edit?usp=sharing): Presentation slides that people are welcome to use when giving talks. Please give appropriate credit if you do!
* [SKA precursor conf](https://www.youtube.com/watch?v=DKCBm5TdJW0&t=1s): A 10-min talk on SETI search with SKA precursors
* [SKA Crable of Life seminar](https://www.dropbox.com/s/cap4b8g95axqg33/Ng_SKACoL_Webinar_06052021.mp4?dl=0): Hour-long talk on MeerKAT search 
