<?php
// informações de conexão com o banco de dados
	$servidor = "localhost";
	$usuario = "root";
	$senha = "112004";
	$dbnome = "TrabalhoTCC";

$conn = mysqli_connect($servidor, $usuario, $senha, $dbnome);

$sqlconsulta =  "select * from responsavel where id = 1 ";

$result = $conn->query($sqlconsulta);
if ($result->num_rows > 0) {
	while($row = $result->fetch_assoc()) {
		$saida = "id: " . $row["id"];
		$saida = $saida . " - Nome: " . $row["nome"];
		$saida = $saida . " - Email: " . $row["email"] ;
		$saida = $saida . "- ". $row["reg_date"] . " ";
		$saida = $saida . "<br>";

		/*** INICIO - DADOS A SEREM ALTERADOS DE ACORDO COM SUAS CONFIGURAÇÕES DE E-MAIL ***/ 
		$enviaFormularioParaNome = $row['nome']; //'Nome do destinatário que receberá formulário';
		$enviaFormularioParaEmail = $row['email']; //'email-do-destinatario@dominio';

		$caixaPostalServidorNome = 'nodemcutcc@gmail.com'; //'WebSite | Formulário';
		$caixaPostalServidorEmail = 'nodemcutcc@gmail.com'; //'usuario@seu-dominio';
		$caixaPostalServidorSenha = 'chapolincolorado'; //'senha';
 
		/*** FIM - DADOS A SEREM ALTERADOS DE ACORDO COM SUAS CONFIGURAÇÕES DE E-MAIL ***/ 

		/* abaixo as veriaveis principais, que devem conter em seu formulario*/
 		$mensagemConcatenada = 'Email gerado do seu NodeMCU'.'<br>'; 
		$mensagemConcatenada .= '----------------------------------------------------------------------<br><br/>'; 
		$mensagemConcatenada .= 'Foi detectado uma queda de seu NodeMCU'.'<br>'; 
		$mensagemConcatenada .= 'Entre em contato com os idosos de sua residência'.'<br>'; 
		$mensagemConcatenada .= 'Muito Obrigada!'.'<br>';
		$mensagemConcatenada .= '------------------------------------------------------------------------<br><br>'; 
		$mensagemConcatenada .= 'Email Automático, Por favor não Responder.'.'<br/>';
	 
	 
		/*********************************** A PARTIR DAQUI NAO ALTERAR ************************************/ 
 
		require_once('PHPMailer-master/PHPMailerAutoload.php');

		$mail = new PHPMailer();
		$mail->IsSMTP();
		$mail->SMTPAuth  = true;
		$mail->SMTPSecure = 'tls';	// SSL REQUERIDO pelo GMail
		$mail->Charset   = 'utf8_decode()';
		$mail->Host  = 'smtp.'.substr(strstr($caixaPostalServidorEmail, '@'), 1);
		$mail->Port  = '587';
		$mail->Username  = $caixaPostalServidorEmail;
		$mail->Password  = $caixaPostalServidorSenha;
		$mail->From  = $caixaPostalServidorEmail;
		$mail->FromName  = utf8_decode($caixaPostalServidorNome);
		$mail->IsHTML(true);
		$mail->Subject  = utf8_decode('Possível Queda!');
		$mail->Body  = utf8_decode($mensagemConcatenada);

		$mail->AddAddress($enviaFormularioParaEmail,utf8_decode($enviaFormularioParaNome));

		if(!$mail->Send()){
			$mensagemRetorno = 'Erro ao enviar email: '. print($mail->ErrorInfo);
		}
		else{
			$mensagemRetorno = 'Email enviado com sucesso!';
		}
	} 
}

?>
