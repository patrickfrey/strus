git checkout gh-pages 
cd doc/
git pull origin master 
git rm doxygen/html/*
doxygen doxygen.conf
git status
git add doxygen/html/*
cd ..
git commit -m "updated interface documentation (version 0.17)"
git push origin gh-pages 
git checkout master 



