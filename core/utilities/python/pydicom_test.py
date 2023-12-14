import pydicom
import numpy as np
path = "/DICOM/"
name = '000000.dcm'
def run(a):
    ds = pydicom.dcmread(a+path+name)
    BeamCounter = ds[0x300a, 0x0070][0][0x300a, 0x0080].value
    NumOfLeaves = ds[0x300a,0x00b0][0][0x300a,0x0111][0][0x300a,0x011a][2][0x300a,0x011c].VM
    flag=0
    for k in range(0,BeamCounter):
        BeamDose = ds[0x300a, 0x0070][0][0x300c, 0x0004][k][0x300a, 0x0084].value
        print('In',k+1,'beam we hawe',BeamDose,'Gy.')
        flag=flag+BeamDose
    print('\nTotal dose',flag,'\n')
    for i in range(0,BeamCounter):
        ControlPointsCounter = ds[0x300a,0x00b0][i][0x300a,0x0110].value
        print('\n\nIn',i+1,'beam we haw',ControlPointsCounter,'control points.')
        # Geometry
        for j in range(ControlPointsCounter-3,ControlPointsCounter-2):
            # # Angle

            # # Jaws
            JawChangeCheck = ds[0x300a,0x00b0][i][0x300a,0x0111][j][0x300a,0x011a].VM
            if JawChangeCheck==3:
                print("\nJaws possition in a",i+1,'beam')
                print('— The \'X\' jaw',  )
                flag = ds[0x300a,0x00b0][i][0x300a,0x0111][0][0x300a,0x011a][0][0x300a,0x011c].value
                JawXPositions = np.zeros(2,np.single)
                for k in range(0,2):
                    JawXPositions[k] = flag.pop(0)
                print(JawXPositions[0],JawXPositions[1])
                print('— The \'Y\' jaw')
                flag = ds[0x300a,0x00b0][i][0x300a,0x0111][0][0x300a,0x011a][1][0x300a,0x011c].value
                JawYPositions = np.zeros(2,np.single)
                for k in range(0,2):
                    JawYPositions[k] = flag.pop(0)
                print(JawYPositions[0],JawYPositions[1])
            # # MLCX possition
            if JawChangeCheck==3:
              flag = ds[0x300a,0x00b0][i][0x300a,0x0111][j][0x300a,0x011a][2][0x300a,0x011c].value
              LeavesPositions = np.zeros(NumOfLeaves,np.single)
              print('\nThe MLC pair possition in a',i+1,'beam and the',j+1,'control point:')
              for k in range(0,NumOfLeaves):
                  LeavesPositions[k] = flag.pop(0)
              for k in range(0,60):
                print('The',k+1,'pair of MLC:',LeavesPositions[k],'and',LeavesPositions[k+60])
            if JawChangeCheck==1:
              flag = ds[0x300a,0x00b0][i][0x300a,0x0111][j][0x300a,0x011a][0][0x300a,0x011c].value
              LeavesPositions = np.zeros(NumOfLeaves,np.single)
              print('\nThe MLC pair possition in a',i+1,'beam and the',j+1,'control point:')
              for k in range(0,NumOfLeaves):
                  LeavesPositions[k] = flag.pop(0)
              for k in range(0,60):
                  print('The',k+1,'pair of MLC:',LeavesPositions[k],'and',LeavesPositions[k+60])
    print('Done!')
