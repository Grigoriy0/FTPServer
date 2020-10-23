resource "aws_subnet" "subnet_1" {
  vpc_id     = aws_vpc.ftp_vpc.id
  cidr_block = "10.0.3.0/24"

  tags = {
    Name = "rds subnet 1"
  }
}

resource "aws_subnet" "subnet_2" {
  vpc_id     = aws_vpc.ftp_vpc.id
  cidr_block = "10.0.4.0/24"

  tags = {
    Name = "rds subnet 2"
  }
}
