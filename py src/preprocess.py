from sklearn import preprocessing
from sklearn.preprocessing import StandardScaler
import numpy as np
import os.path
import glob


# Initiate  X
def loadX(path,feature_size):
    with open(path) as f:
        lines = f.readlines()
    X=np.zeros((len(lines),feature_size))
    for i in range(len(lines)):
        line=lines.pop(0)
        splites=line.split()
        while (len(splites) < feature_size):
            splites.append(100)
        words = np.array(splites)
        #print(line)
        #for now we have 16 features
        #print(line)

        for j in range(feature_size):
            X[i,j] = words[j]

    l=0
    while( l< X.shape[0]-1):
        while(l< X.shape[0]-1 and X[l+1,0]-X[l,0] < 10 and X[l+1,0]-X[l,0] > 0):
            X = np.delete(X,l,axis=0)
            continue
        l+=1
    return X

# Initiate  _y
def loady(path,X):
    _y=[]
    count=0
    with open(path) as f:
        lines = f.readlines()
        for i in range(len(lines)-1):
            if(i < len(lines)-1 and lines[i] == lines[i+1] ):
                count+=1
                if(i<len(lines)-2):
                    continue
            if(count<3):
                continue
            line = lines[i]
            words = np.array(line.split())
            _yi=[]
            _yi.append(float(words[0]))
            _yi.append(float(words[1]))
            _yi.append(count)
            _y.append(_yi)
            count=0
    _y=np.array(_y).reshape((-1,3))

    #make y
    y=np.zeros((X.shape[0],))

    index=0
    for i in range(X.shape[0]):
        #print(X[i, 0],_y[index, 0],_y[index+1, 0])
        if index<len(_y) and (_y[index,0]-X[i,0]) < 15 :
            y[i]=1
            #print("goalllll", _y[index, 0], index+1)
            index+=1

    verified=True
    if len(_y)!=np.sum(y) or index<len(_y) :
        verified=False
        #print("discrepancy in ",path, "index:",index, "goals",np.sum(y))

    #print("Real goals for ",path," ---> ",len(_y))
    return verified,y



def load_scaled_data():
    X ,y=loadData()
    #print("X:",X.shape)
    #print("y:", y.shape)
    scaler = StandardScaler()
    print("Data Standardization by Centering and Scaling")
    print(scaler.fit(X))
    print("************************************")
    #X_scaled = preprocessing.scale(X)
    X_scaled=scaler.transform(X)
    return X_scaled,y


def loadData(feature_size=16):
    X_stack=[]
    y_stack=[]
    goals= glob.glob("./data/goals*.txt")
    shoots=glob.glob("./data/shoots*.txt")
    counter=0
    rejected=0

    for i in range(len(shoots)):
        if not os.path.isfile(shoots[i].replace("shoots","goals")):
            print(shoots[i].replace("shoots","goals"))

    if len(goals)!= len(shoots):
        print("Goals:",len(goals),"!= Shoots:",len(shoots))
        exit(0)


    for i in range(len(goals)):
        #print("X file:",shoots[i])
        part_X = loadX(shoots[i], feature_size)
        goals_path = shoots[i].replace("shoots", "goals")
        #print("y file:", goals_path)
        verified, part_y = loady(goals_path,part_X)
        if not verified:
            #print("just passed", shoots[i])
            rejected+=1
            continue
        else :
            counter+=1
            X_stack.append(part_X)
            y_stack.append(part_y)

    X=np.concatenate(X_stack, axis=0)
    y=np.concatenate(y_stack, axis=0)

    if X.shape[0]!=y.shape[0] :
        print("discrepancy in results", "shoots:",X.shape[0], "results:",y.shape[0])
    goals = np.sum(y)
    non_goals = len(y) - goals
    print("Files checked:",counter,"Files with discrepancy:",rejected," Scored:", goals, " Not Scored:", non_goals)
    return X,y

if __name__ == '__main__':

    #X,y=loadData(feature_size=8)
    X,y=loadData(16)
