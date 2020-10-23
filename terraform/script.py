
if __name__ == "__main__":
    try:
        f_in = open("ip")
    except FileNotFoundError:
        print('File not found. skip')
    else:
        ip = f_in.read().strip()
        f_in.close()

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
