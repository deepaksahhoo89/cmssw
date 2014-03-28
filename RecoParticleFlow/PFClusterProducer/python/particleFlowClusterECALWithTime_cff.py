import FWCore.ParameterSet.Config as cms
from RecoParticleFlow.PFClusterProducer.particleFlowRecHitECALWithTime_cfi import *
from RecoParticleFlow.PFClusterProducer.particleFlowClusterECALWithTimeUncorrected_cfi import *
from RecoParticleFlow.PFClusterProducer.particleFlowClusterECALWithTimeSelected_cfi import *

from RecoParticleFlow.PFClusterProducer.particleFlowClusterECAL_cfi import *


particleFlowClusterECALSequence = cms.Sequence(
    particleFlowRecHitECALWithTime+
    particleFlowClusterECALWithTimeUncorrected +
    particleFlowClusterECALWithTimeSelected +
    particleFlowClusterECAL
    )
                                                   
