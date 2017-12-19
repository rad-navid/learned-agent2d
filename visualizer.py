import matplotlib.pyplot as plt
from matplotlib import offsetbox
import numpy as np
import preprocess


def tsne_impl():
    from sklearn.manifold import TSNE
    X,y=preprocess.loadData()
    X_embedded = TSNE(n_components=2).fit_transform(X)
    print(X_embedded.shape)
    colors = np.random.rand(X_embedded.shape[0])
    scored_indices= y==1
    not_scored_indices=y==0
    fig, ax = plt.subplots()
    ax.scatter(X_embedded[scored_indices,0], X_embedded[scored_indices,1], c='red',label='Scored', marker='*')
    ax.scatter(X_embedded[not_scored_indices, 0], X_embedded[not_scored_indices, 1],c='blue',label='Not scored', marker='+')
    ax.legend()
    plt.show()

def PCA():
    from sklearn.decomposition import PCA
    X, y = preprocess.loadData()
    pca = PCA(n_components=2, svd_solver='full')
    X_embedded=pca.fit_transform(X,y)
    print(X.shape)
    print(X_embedded.shape)
    colors = np.random.rand(X_embedded.shape[0])
    scored_indices = y == 1
    not_scored_indices = y == 0
    fig, ax = plt.subplots()
    ax.scatter(X_embedded[scored_indices, 0], X_embedded[scored_indices, 1], c='red', label='Scored', marker='*')
    ax.scatter(X_embedded[not_scored_indices, 0], X_embedded[not_scored_indices, 1], c='blue', label='Not scored',
               marker='+')
    ax.legend()
    plt.show()

def visualize(X, y):

    feature_names=['time', 'ball.x', 'ball.y', 'target.x',
              'target.y', 'ball.vel', 'goali_line_dist', 'goali_ball_dist',
              'opp line dist 1','opp line dist 2','opp line dist 3','opp line dist 4',
            'opp ball dist 1', 'opp ball dist 2','opp ball dist 3','opp ball dist 4']

    #feature_names = ['time', 'goal angle', 'goalie distance', 'def opps','opp dist 1','opp dist 2', 'dist ratio 1', 'dist ratio 2']

    plt.figure(figsize=(20, 10))
    feature_count = X.shape[1]
    print(feature_count, len(feature_names))
    # i: index
    for i in range(feature_count):
        plt.subplot(4, 4, i + 1)
        plt.ylabel("Score")
        plt.xlabel(feature_names[i])
        plt.scatter(X[:, i], y)

    plt.tight_layout()
    plt.show()


def density_plot(score,no_score):
    import seaborn as sns
    # plot of 2 variables
    p1 = sns.kdeplot(score, shade=True, color="r",legend=True)
    p2 = sns.kdeplot(no_score, shade=True, color="b",legend=True)
    plt.legend(('Scored','Not Scored'), ncol=2, loc='upper left')
    sns.plt.show()
if __name__ == '__main__':
    X, y = preprocess.load_scaled_data()
    #tsne_impl()
    #PCA()

    #visualize(X, y)
    score_X = X[y == 1]
    no_score_X = X[y == 0]
    density_plot(score_X[:, 1], no_score_X[:, 1])