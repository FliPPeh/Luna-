-- Just to make sure this isn"t forgotten
print("Default configuration not changed! Please edit config.lua to change " ..
      "connection and user settings")

os.exit(1)

-- User info
nick = "changeme"
user = "changeme"
realname = "changeme"

-- Server info
server_addr = "changeme"
server_port = 6697
server_password = ""

ssl = true

scripts = {"scriptloader", "base"}
autojoin = {}

