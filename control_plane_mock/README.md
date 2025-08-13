# Setup
sudo apt install nodejs
sudo apt install npm 

npm install swagger-mock-api
npm install grunt grunt-contrib-connect swagger-mock-api

curl -o- https://raw.githubusercontent.com/nvm-sh/nvm/v0.39.7/install.sh | bash
nvm install 22
nvm use 22

# Execute
npx @redocly/cli preview -d api
