

resource "aws_security_group" "ftp_ssh" {
  name        = "allow_ftp_ssh"
  description = "Allow 21, 22, 2000-5000"

  ingress { # ftp port
    from_port   = 21
    to_port     = 21
    protocol    = "tcp"
    cidr_blocks = ["0.0.0.0/0"]
  }

  ingress { # ssh port
    from_port   = 22
    to_port     = 22
    protocol    = "tcp"
    cidr_blocks = ["0.0.0.0/0"]
  }

  ingress { # ports for ftp data threads
    from_port   = 2000
    to_port     = 3000
    protocol    = "tcp"
    cidr_blocks = ["0.0.0.0/0"]
  }

  egress {
    from_port   = 0
    to_port     = 0
    protocol    = "-1"
    cidr_blocks = ["0.0.0.0/0"]
  }

}
