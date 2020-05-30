
if __name__ == "__main__":
    f_in = open("ip.txt")
    ip = f_in.read().strip()
    f_in.close()

    if len(ip) == 0:
        print('file ip.txt is empty')
        exit(1)

    server = ip.strip()+\
    " ansible_connection=ssh"+\
    " ansible_user=ubuntu"+\
    " ansible_ssh_private_key="+\
    "~/.ssh/terraform"

    f_out = open("../hosts.ini", 'w')

    f_out.write("[FTP_SERVERS]\n")
    f_out.write(server)

    f_out.write("\n\n[DATABASE_SERVERS]\n")
    f_out.write(server)

    f_out.close()
