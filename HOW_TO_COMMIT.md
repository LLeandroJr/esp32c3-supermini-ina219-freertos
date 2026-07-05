
git init

git remote add origin https://github.com/LLeandroJr/esp32c3-supermini-ina219-freertos.git

git remote remove origin

git remote -v

git config pull.rebase false

git pull origin main --allow-unrelated-histories

git add files

git status

git commit -m "mensagem"

git branch -M main

git push -u origin main