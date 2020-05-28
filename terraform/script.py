
if __name__ == "__main__":
    f_in = open("ip.txt")
    ip = f_in.read()
    f_in.close()

    server = ip.strip()+\
    " ansible_connection=ssh"+\
    " ansible_user=ubuntu"+\
    " ansible_ssh_private_key="+\
    "~/.ssh/aws_key_pair.pem"

    f_out = open("../hosts.ini", 'w')

    f_out.write("[FTP_SERVERS]\n")
    f_out.write(server)

    f_out.write("\n\n[DATABASE_SERVERS]\n")
    f_out.write(server)

    f_out.close()