import pydicom
import numpy as np
name = 'example-vmat.dcm'


def return_possition(dataFile, side, current_beam, current_controlpoint, num_of_leaves):
    ds = pydicom.dcmread(dataFile)
    AllLeavesPositions = np.zeros(num_of_leaves, np.single)
    SideLeavesPositions = np.zeros(int(num_of_leaves/2), np.single)
    if current_controlpoint == 0:
        if side == "Y1":
            AllLeavesPositions = ds[0x300a, 0x00b0][current_beam][0x300a, 0x0111][current_controlpoint][0x300a, 0x011a][2][0x300a, 0x011c].value
            SideLeavesPositions = AllLeavesPositions[0:int(num_of_leaves/2)]
        if side == "Y2":
            AllLeavesPositions = ds[0x300a, 0x00b0][current_beam][0x300a, 0x0111][current_controlpoint][0x300a, 0x011a][2][0x300a, 0x011c].value
            SideLeavesPositions = AllLeavesPositions[int(num_of_leaves/2):int(num_of_leaves)]
    else:
        AllLeavesPositions = ds[0x300a, 0x00b0][current_beam][0x300a, 0x0111][current_controlpoint][0x300a, 0x011a][0][0x300a, 0x011c].value
        if side == "Y1":
            AllLeavesPositions = ds[0x300a, 0x00b0][current_beam][0x300a, 0x0111][current_controlpoint][0x300a, 0x011a][0][0x300a, 0x011c].value
            SideLeavesPositions = AllLeavesPositions[0:int(num_of_leaves/2)]
        if side == "Y2":
            AllLeavesPositions = ds[0x300a, 0x00b0][current_beam][0x300a, 0x0111][current_controlpoint][0x300a, 0x011a][0][0x300a, 0x011c].value
            SideLeavesPositions = AllLeavesPositions[0:int(num_of_leaves/2)]
    return SideLeavesPositions


def return_number_of_beams(dataFile):
    return pydicom.dcmread(dataFile)[0x300a, 0x0070][0][0x300a, 0x0080].value


def return_number_of_controlpoints(dataFile, beam_number):
    return pydicom.dcmread(dataFile)[0x300a, 0x00b0][beam_number][0x300a, 0x0110].value

def return_number_of_leaves(dataFile):
    ds = pydicom.dcmread(dataFile)
    number_of_leaves = ds[0x300a, 0x00b0][0][0x300a, 0x0111][0][0x300a, 0x011a][2][0x300a, 0x011c].VM
    return number_of_leaves
