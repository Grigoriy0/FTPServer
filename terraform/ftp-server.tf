
resource "aws_instance" "ftp-server" {
  ami           = var.ftp_server_ami
  instance_type = "t2.micro"
  key_name      = aws_key_pair.my_key_pair.key_name

  user_data = file("install_python.sh")    # for ansible

  vpc_security_group_ids = [aws_security_group.ftp_ssh.id]
  
  tags = {
    name = "Ubuntu-ftp-server"
  }

  provisioner "local-exec" {    # for ansible
    command = "echo ${aws_instance.ftp-server.public_ip} > ip.txt && python3 script.py"
  }

  provisioner "remote-exec" {
    connection {
      type = "ssh"
      user = var.ftp_server_user
      private_key = file("~/.ssh/terraform")
      host = self.public_ip
    }

    inline = [
      "echo ${aws_instance.ftp-server.public_ip} > ./my_ftp_pub_ip.txt"
    ]
  }
}

resource "aws_key_pair" "my_key_pair" {
  key_name = "aws_key_pair"
  public_key = file("~/.ssh/terraform.pub")
}


output "ftp_ip" {
  value = aws_instance.ftp-server.public_ip
}

