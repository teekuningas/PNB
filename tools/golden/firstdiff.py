# firstdiff.py A B — the first frame where two golden traces differ, and which sections moved.
import sys
names=["players","ball","pII","pRAI","flow","pitch","throw","swing","rules","client","ai","out"]
a=open(sys.argv[1]).read().split('\n'); b=open(sys.argv[2]).read().split('\n')
for la,lb in zip(a,b):
    if la!=lb:
        if la.startswith('#') or lb.startswith('#'):
            print('meta:',la,'|',lb); break
        fa=la.split(); fb=lb.split()
        print('frame',fa[0],'sections:',[names[i] for i in range(12) if fa[i+1]!=fb[i+1]]); break
else:
    print('identical prefix; lengths',len(a),len(b))
