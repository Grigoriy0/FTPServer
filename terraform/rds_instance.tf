
resource "aws_db_instance" "db_ftp" {
  depends_on             = [aws_security_group.rds_sg]
  allocated_storage      = "10"
  engine                 = "mysql"
  engine_version         = "5.7.21"
  instance_class         = "db.t2.micro"
  name                   = var.db_name
  username               = var.username
  password               = var.password
  vpc_security_group_ids = [aws_security_group.rds_sg.id]
  db_subnet_group_name   = aws_db_subnet_group.db_ftp_subnet.id
  skip_final_snapshot    = true
}

resource "aws_db_subnet_group" "db_ftp_subnet" {
  name        = "rds_subnet"
  description = "description"
  subnet_ids  = [aws_subnet.subnet_1.id, aws_subnet.subnet_2.id]
}
