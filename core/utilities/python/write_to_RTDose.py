import os
import pydicom
import numpy as np
from pydicom.dataset import Dataset, FileMetaDataset
from pydicom.uid import UID, ExplicitVRLittleEndian
from pydicom.sequence import Sequence

def write_rtdose(metadata, dose_grid, output_file):
   pass