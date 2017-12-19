
from sklearn.model_selection import train_test_split
from sklearn.metrics import accuracy_score, f1_score, precision_score, recall_score
import preprocess
import numpy as np


def load_data(test_size):
    X, y = preprocess.load_scaled_data()
    #X, y = preprocess.loadData()
    print(X.shape, y.shape)
    X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=test_size, random_state=42)
    print("Train Data Count:",X_train.shape[0])
    print("Test Data Count:", X_test.shape[0])
    print("************************************")
    return X_train, X_test, y_train, y_test

def SGDClassifier():
    from sklearn import linear_model
    X_train, X_test, y_train, y_test=load_data(.33)
    clf = linear_model.SGDClassifier(loss ='squared_loss',penalty='l1')
    clf.fit(X_train, y_train)
    print(clf)
    test_pred = clf.predict(X_test)
    train_pred= clf.predict(X_train)
    cal_metrics(y_train,y_test,train_pred,test_pred)

def logistic_regression():
    from sklearn.linear_model import LogisticRegression
    X_train, X_test, y_train, y_test=load_data(.33)
    LogReg = LogisticRegression(penalty='l1')
    LogReg.fit(X_train, y_train)
    print(LogReg)
    #print("COEFF:",LogReg.coef_)
    #print("bias",LogReg.intercept_)
    test_pred = LogReg.predict(X_test)
    train_pred= LogReg.predict(X_train)
    cal_metrics(y_train,y_test,train_pred,test_pred)

def decision_tree_classifier():
    from sklearn import tree
    X_train, X_test, y_train, y_test = load_data(.33)
    clf = tree.DecisionTreeClassifier()
    print(clf)
    clf.fit(X_train, y_train)
    test_pred = clf.predict(X_test)
    train_pred = clf.predict(X_train)
    cal_metrics(y_train, y_test, train_pred, test_pred)

def knn():
    from sklearn.neighbors import KNeighborsClassifier
    X_train, X_test, y_train, y_test = load_data(.33)
    accuracy_list = np.zeros((9,))
    for i in range(1, 10):
        neigh = KNeighborsClassifier(n_neighbors=i,metric='minkowski')
        neigh.fit(X_train, y_train)
        y_pred = neigh.predict(X_test)
        accuracy_list[i - 1] = accuracy_score(y_test, y_pred)

    max_k = accuracy_list.argmax() + 1;
    print("Optimum K:", max_k)
    clf = KNeighborsClassifier(n_neighbors=max_k,metric='minkowski')
    clf.fit(X_train, y_train)
    print(clf)
    test_pred = clf.predict(X_test)
    train_pred = clf.predict(X_train)
    cal_metrics(y_train, y_test, train_pred, test_pred)

def svm_model():
    from sklearn import svm
    X_train, X_test, y_train, y_test = load_data(.33)
    #this setting is for 8 features
    #clf = svm.SVC(C=.5, kernel='poly', degree=10, gamma='auto')
    # this set up is for 16 features setting
    clf = svm.SVC(C=1, kernel='poly', degree=5, gamma='auto')
    clf.fit(X_train, y_train)
    test_pred = clf.predict(X_test)
    train_pred = clf.predict(X_train)
    cal_metrics(y_train, y_test, train_pred, test_pred)

def gaussian_mixture():
    from sklearn import mixture
    X_train, X_test, y_train, y_test = load_data(.33)
    clf = mixture.GaussianMixture(covariance_type='full')
    clf.fit(X_train, y_train)
    test_pred = clf.predict(X_test)
    train_pred = clf.predict(X_train)
    cal_metrics(y_train, y_test, train_pred, test_pred)

def MLPClassifier():
    from sklearn.neural_network import MLPClassifier
    X_train, X_test, y_train, y_test = load_data(.33)
    # this setting is for 8 features
    #clf = MLPClassifier(solver='lbfgs',activation='tanh', alpha=1e-5,hidden_layer_sizes = (9,7,5,4,2), random_state = 1)
    #this setting is for 16 features
    clf = MLPClassifier(solver='lbfgs', activation='tanh', alpha=1e-5, hidden_layer_sizes=(17,8,4,2),random_state=1)
    clf = clf.fit(X_train, y_train)
    print(clf)
    test_pred = clf.predict(X_test)
    train_pred = clf.predict(X_train)
    cal_metrics(y_train, y_test, train_pred, test_pred)

def cal_metrics(y_train,y_test,train_pred,test_pred):
    # Calculate Accuracy Rate by using accuracy_score()
    print("Train Accuracy ",accuracy_score(y_train,train_pred ))
    print("Test Accuracy", accuracy_score(y_test, test_pred))
    print("F1 Score:",f1_score(y_test, test_pred, average="macro"))
    print("Precision Score:",precision_score(y_test, test_pred, average="macro"))
    print("Recall Score:",recall_score(y_test, test_pred, average="macro"))

if __name__ == '__main__':

    #SGDClassifier()
    logistic_regression()
    #svm_model()
    #gaussian_mixture()
    #decision_tree_classifier()
    #MLPClassifier()
    #knn()